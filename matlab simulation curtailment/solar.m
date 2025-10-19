function solar(block)
% Level-2 MATLAB S-Function for real-time ThingsBoard communication
setup(block);
end

function setup(block)
    % Register number of ports
    block.NumInputPorts  = 2;  % voltage and current inputs
    block.NumOutputPorts = 1;  % breaker command output

    % Setup port properties
    block.SetPreCompInpPortInfoToDynamic;
    block.SetPreCompOutPortInfoToDynamic;

    block.InputPort(1).Dimensions = 1;
    block.InputPort(1).DatatypeID = 0;  % double
    block.InputPort(1).Complexity = 'Real';
    block.InputPort(1).DirectFeedthrough = false;

    block.InputPort(2).Dimensions = 1;
    block.InputPort(2).DatatypeID = 0;  % double
    block.InputPort(2).Complexity = 'Real';
    block.InputPort(2).DirectFeedthrough = false;

    block.OutputPort(1).Dimensions = 1;
    block.OutputPort(1).DatatypeID = 0;  % double
    block.OutputPort(1).Complexity = 'Real';

    % Register sample times
    block.SampleTimes = [0.5 0];  % Sample every 0.5 seconds

    % Register methods
    block.RegBlockMethod('PostPropagationSetup', @DoPostPropSetup);
    block.RegBlockMethod('InitializeConditions', @InitializeConditions);
    block.RegBlockMethod('Outputs', @Outputs);
    block.RegBlockMethod('Update', @Update);
    block.RegBlockMethod('Terminate', @Terminate);
end

function DoPostPropSetup(block)
    block.NumDworks = 3;
    block.Dwork(1).Name = 'last_send_time';
    block.Dwork(1).Dimensions = 1;
    block.Dwork(1).DatatypeID = 0;
    block.Dwork(1).Complexity = 'Real';

    block.Dwork(2).Name = 'breaker_state';
    block.Dwork(2).Dimensions = 1;
    block.Dwork(2).DatatypeID = 0;
    block.Dwork(2).Complexity = 'Real';

    block.Dwork(3).Name = 'last_check_time';
    block.Dwork(3).Dimensions = 1;
    block.Dwork(3).DatatypeID = 0;
    block.Dwork(3).Complexity = 'Real';
end

function InitializeConditions(block)
    block.Dwork(1).Data = 0;  % last_send_time
    block.Dwork(2).Data = 1;  % breaker_state (1 = closed/on)
    block.Dwork(3).Data = 0;  % last_check_time
end

function Outputs(block)
    % Output current breaker state
    block.OutputPort(1).Data = block.Dwork(2).Data;
end

function Update(block)
    current_time = block.CurrentTime;
    voltage = block.InputPort(1).Data;
    current = block.InputPort(2).Data;

    % Send telemetry data periodically
    if current_time - block.Dwork(1).Data >= 0.5
        try
            % Prepare telemetry data
            telemetry = struct();
            telemetry.voltage = voltage;
            telemetry.current = current;
            telemetry.power = voltage * current;
            telemetry.simulation_time = current_time;
            telemetry.breaker_state = block.Dwork(2).Data;

            % Send to ThingsBoard
            url = 'http://demo.thingsboard.io/api/v1/51laFCK7CbO4wgArDuLs/telemetry';
            options = weboptions('MediaType', 'application/json', ...
                                'RequestMethod', 'POST', ...
                                'Timeout', 5, ...
                                'ContentType', 'json');
            webwrite(url, jsonencode(telemetry), options);

            block.Dwork(1).Data = current_time;
        catch
            % Silently continue if send fails
        end
    end

    % Check for breaker commands periodically
    if current_time - block.Dwork(3).Data >= 1.0
        try
            url = 'http://demo.thingsboard.io/api/v1/51laFCK7CbO4wgArDuLs/attributes?sharedKeys=breaker_command';
            options = weboptions('MediaType', 'application/json', ...
                                'Timeout', 5);
            response = webread(url, options);

            if isfield(response, 'shared') && isfield(response.shared, 'breaker_command')
                breaker_cmd = response.shared.breaker_command;
                if breaker_cmd == 1
                    block.Dwork(2).Data = 1;  % Close breaker
                else
                    block.Dwork(2).Data = 0;  % Open breaker
                end
            end

            block.Dwork(3).Data = current_time;
        catch
            % Silently continue if check fails
        end
    end
end

function Terminate(block)
    fprintf('ThingsBoard communication terminated\\n');
end