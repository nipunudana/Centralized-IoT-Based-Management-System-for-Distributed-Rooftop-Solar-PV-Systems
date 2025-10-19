%% Quick Test: Is MPPT Causing Voltage Variation?
% This script tests if MPPT is responsible for voltage changes

clear; clc;

fprintf('═══════════════════════════════════════════════════════\n');
fprintf('   MPPT VOLTAGE VARIATION TEST\n');
fprintf('═══════════════════════════════════════════════════════\n\n');

MODEL_NAME = 'Home_solar_power_gen2021';

%% Test Configuration
irradiance_levels = [200, 600, 1000];  % W/m²
test_duration = 1.5;  % seconds

%% Load model
try
    load_system(MODEL_NAME);
    fprintf('✓ Model loaded\n\n');
catch ME
    fprintf('❌ Error loading model: %s\n', ME.message);
    return;
end

set_param(MODEL_NAME, 'StopTime', num2str(test_duration));

%% Run tests
fprintf('Running MPPT analysis...\n');
fprintf('━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n');

results = struct('irr', {}, 'v', {}, 'i', {}, 'p', {});

for idx = 1:length(irradiance_levels)
    irr = irradiance_levels(idx);
    fprintf('Testing at %d W/m²... ', irr);
    
    try
        % Set irradiance (try multiple methods)
        assignin('base', 'Irradiance', irr);
        
        % Try to find and set constant block
        try
            irr_blocks = find_system(MODEL_NAME, 'BlockType', 'Constant');
            for i = 1:length(irr_blocks)
                block_name = get_param(irr_blocks{i}, 'Name');
                if contains(lower(block_name), 'irr', 'IgnoreCase', true)
                    set_param(irr_blocks{i}, 'Value', num2str(irr));
                    break;
                end
            end
        catch
        end
        
        % Run simulation
        sim_out = sim(MODEL_NAME, 'ReturnWorkspaceOutputs', 'on');
        
        % Get data
        voltage = [];
        current = [];
        
        if isfield(sim_out, 'voltage_data')
            voltage = sim_out.voltage_data;
        elseif evalin('base', 'exist(''voltage_data'', ''var'')')
            voltage = evalin('base', 'voltage_data');
        end
        
        if isfield(sim_out, 'current_data')
            current = sim_out.current_data;
        elseif evalin('base', 'exist(''current_data'', ''var'')')
            current = evalin('base', 'current_data');
        end
        
        if ~isempty(voltage) && ~isempty(current)
            % Use last 40% of data (steady state)
            start_idx = round(length(voltage) * 0.6);
            
            results(idx).irr = irr;
            results(idx).v = mean(voltage(start_idx:end));
            results(idx).i = mean(current(start_idx:end));
            results(idx).p = results(idx).v * results(idx).i;
            
            fprintf('V=%.2fV, I=%.2fA, P=%.1fW ✓\n', ...
                    results(idx).v, results(idx).i, results(idx).p);
        else
            fprintf('No data ❌\n');
        end
        
    catch ME
        fprintf('Failed: %s ❌\n', ME.message);
    end
end

%% Analysis
if length(results) >= 2
    fprintf('\n');
    fprintf('═══════════════════════════════════════════════════════\n');
    fprintf('   ANALYSIS\n');
    fprintf('═══════════════════════════════════════════════════════\n\n');
    
    % Calculate changes
    v_low = results(1).v;
    v_high = results(end).v;
    v_change_percent = abs(v_high - v_low) / v_high * 100;
    
    i_low = results(1).i;
    i_high = results(end).i;
    i_ratio = (i_high / i_low) / (results(end).irr / results(1).irr);
    
    p_low = results(1).p;
    p_high = results(end).p;
    p_ratio = (p_high / p_low) / (results(end).irr / results(1).irr);
    
    % Display summary
    fprintf('Voltage Analysis:\n');
    fprintf('  At %d W/m²: %.2f V\n', results(1).irr, v_low);
    fprintf('  At %d W/m²: %.2f V\n', results(end).irr, v_high);
    fprintf('  Change: %.2f V (%.1f%%)\n\n', v_high - v_low, v_change_percent);
    
    fprintf('Current Scaling:\n');
    fprintf('  Ratio: %.2f (should be ~1.0 for proper scaling)\n\n', i_ratio);
    
    fprintf('Power Scaling:\n');
    fprintf('  Ratio: %.2f (should be ~1.0 for max power tracking)\n\n', p_ratio);
    
    % Diagnosis
    fprintf('═══════════════════════════════════════════════════════\n');
    fprintf('   DIAGNOSIS\n');
    fprintf('═══════════════════════════════════════════════════════\n\n');
    
    if v_change_percent < 5
        fprintf('✅ VOLTAGE CHANGE: VERY SMALL (%.1f%%)\n\n', v_change_percent);
        fprintf('This is NOT typical MPPT behavior.\n');
        fprintf('Possible reasons:\n');
        fprintf('  • MPPT is not enabled or not working\n');
        fprintf('  • You are measuring DC bus (regulated) voltage\n');
        fprintf('  • Fixed duty cycle operation\n\n');
        
    elseif v_change_percent < 15
        fprintf('✅ VOLTAGE CHANGE: MODERATE (%.1f%%)\n\n', v_change_percent);
        fprintf('This is NORMAL MPPT behavior! ✓\n\n');
        fprintf('Explanation:\n');
        fprintf('  • MPPT adjusts voltage to track maximum power point\n');
        fprintf('  • MPP voltage changes with irradiance\n');
        fprintf('  • This 5-15%% variation is EXPECTED and CORRECT\n\n');
        
        if p_ratio > 0.9 && p_ratio < 1.1
            fprintf('✅ Power scaling is good (%.2f)\n', p_ratio);
            fprintf('   Your MPPT is working correctly!\n\n');
        else
            fprintf('⚠ Power scaling is off (%.2f)\n', p_ratio);
            fprintf('   MPPT might not be optimizing properly\n\n');
        end
        
        fprintf('CONCLUSION: MPPT is causing voltage variation\n');
        fprintf('             This is CORRECT behavior!\n\n');
        
    else
        fprintf('❌ VOLTAGE CHANGE: LARGE (%.1f%%)\n\n', v_change_percent);
        fprintf('This is MORE than typical MPPT variation.\n');
        fprintf('Possible issues:\n');
        fprintf('  • MPPT algorithm is unstable or oscillating\n');
        fprintf('  • PV model parameters incorrect\n');
        fprintf('  • Measuring at wrong point in circuit\n');
        fprintf('  • Load impedance issues\n\n');
    end
    
    % Check current behavior
    fprintf('Current Behavior:\n');
    if i_ratio > 0.9 && i_ratio < 1.1
        fprintf('✅ Current scales correctly with irradiance (%.2f)\n\n', i_ratio);
    else
        fprintf('⚠ Current scaling is unusual (%.2f)\n', i_ratio);
        fprintf('   Expected: ~1.0 for proportional scaling\n\n');
    end
    
    %% Recommendations
    fprintf('═══════════════════════════════════════════════════════\n');
    fprintf('   RECOMMENDATIONS\n');
    fprintf('═══════════════════════════════════════════════════════\n\n');
    
    if v_change_percent >= 5 && v_change_percent <= 15 && p_ratio > 0.9
        fprintf('🎉 YOUR SYSTEM IS WORKING CORRECTLY!\n\n');
        fprintf('The voltage variation you see is NORMAL for MPPT systems.\n\n');
        
        fprintf('What you should do:\n');
        fprintf('1. ✅ ACCEPT that PV voltage varies with MPPT\n');
        fprintf('2. ✅ Monitor POWER output (should be maximized)\n');
        fprintf('3. ✅ Check DC bus voltage (should be stable)\n');
        fprintf('4. ✅ Update ThingsBoard to show:\n');
        fprintf('      • "PV Operating Voltage" (varies - normal)\n');
        fprintf('      • "DC Bus Voltage" (stable)\n');
        fprintf('      • "Power Output" (maximized)\n');
        fprintf('      • "MPPT Efficiency"\n\n');
        
        fprintf('Optional: Add these calculated values to telemetry:\n\n');
        fprintf('```matlab\n');
        fprintf('telemetry.pv_voltage = voltage;  %% Operating voltage\n');
        fprintf('telemetry.pv_current = current;\n');
        fprintf('telemetry.pv_power = voltage * current;\n');
        fprintf('telemetry.irradiance = current_irradiance;\n');
        fprintf('telemetry.mppt_efficiency = (power/rated_power) * (1000/irradiance) * 100;\n');
        fprintf('```\n\n');
        
    elseif v_change_percent > 15
        fprintf('⚠ VOLTAGE VARIATION IS TOO HIGH\n\n');
        fprintf('Actions to take:\n\n');
        
        fprintf('1. Verify measurement point:\n');
        fprintf('   • Measure voltage at PV terminals (before converter)\n');
        fprintf('   • NOT at DC bus or after inverter\n\n');
        
        fprintf('2. Check MPPT algorithm:\n');
        fprintf('   • What algorithm? (P&O, InCond, etc.)\n');
        fprintf('   • Step size too large?\n');
        fprintf('   • Oscillating around MPP?\n\n');
        
        fprintf('3. Check PV model:\n');
        fprintf('   • Voc, Isc, Vmp, Imp correct?\n');
        fprintf('   • Temperature coefficients set?\n\n');
        
        fprintf('4. Test without MPPT:\n');
        fprintf('   • Use fixed duty cycle = 0.5\n');
        fprintf('   • If voltage stabilizes → MPPT issue\n\n');
        
    else
        fprintf('⚠ VOLTAGE TOO STABLE\n\n');
        fprintf('This suggests MPPT might not be working.\n\n');
        
        fprintf('Check:\n');
        fprintf('1. Is MPPT enabled?\n');
        fprintf('2. Is MPPT block connected?\n');
        fprintf('3. Are you measuring DC bus (regulated) voltage?\n\n');
    end
    
    %% Create comparison plot
    fprintf('\n═══════════════════════════════════════════════════════\n');
    fprintf('   VISUAL COMPARISON\n');
    fprintf('═══════════════════════════════════════════════════════\n\n');
    
    % Create expected vs actual comparison
    figure('Name', 'MPPT Behavior Analysis', 'Position', [100, 100, 1000, 600]);
    
    % Expected MPP voltage curve
    irr_range = 200:100:1000;
    % Typical MPP voltage increases slightly with irradiance
    v_expected = 28 + (irr_range - 200) * 0.003;  % ~2V increase
    
    subplot(2,2,1);
    hold on;
    plot(irr_range, v_expected, '--k', 'LineWidth', 2, 'DisplayName', 'Expected MPP Voltage');
    plot([results.irr], [results.v], '-or', 'LineWidth', 2, 'MarkerSize', 10, 'DisplayName', 'Your Measured Voltage');
    xlabel('Irradiance (W/m²)');
    ylabel('Voltage (V)');
    title('Voltage vs Irradiance');
    legend('Location', 'best');
    grid on;
    hold off;
    
    subplot(2,2,2);
    plot([results.irr], [results.i], '-ob', 'LineWidth', 2, 'MarkerSize', 10);
    xlabel('Irradiance (W/m²)');
    ylabel('Current (A)');
    title('Current vs Irradiance (Should be Linear)');
    grid on;
    
    subplot(2,2,3);
    plot([results.irr], [results.p], '-og', 'LineWidth', 2, 'MarkerSize', 10);
    xlabel('Irradiance (W/m²)');
    ylabel('Power (W)');
    title('Power vs Irradiance (Should be Linear)');
    grid on;
    
    subplot(2,2,4);
    % Efficiency
    rated_power = 250;  % Adjust to your panel rating
    efficiency = ([results.p] ./ rated_power) .* (1000 ./ [results.irr]) * 100;
    plot([results.irr], efficiency, '-om', 'LineWidth', 2, 'MarkerSize', 10);
    xlabel('Irradiance (W/m²)');
    ylabel('Efficiency (%)');
    title('MPPT Efficiency');
    ylim([0, 110]);
    grid on;
    
    fprintf('✓ Plots generated\n\n');
    
else
    fprintf('\n❌ Insufficient data for analysis\n');
    fprintf('Please check that simulation runs and data is logged\n\n');
end

fprintf('═══════════════════════════════════════════════════════\n');
fprintf('TEST COMPLETE\n');
fprintf('═══════════════════════════════════════════════════════\n');