% fabber_get_options.m
%
% Get available Fabber options for a model (and optionally inference method)
%
% If the 'method' input is given, options for this inference method are returned
% as well. If the input 'generic' is given and is 'true', return generic options
% valid for all models/methods
%
% Returns an array of structures, each containing the 
% fields 'name' (option name), 'type' (option type, e.g. 'STR', 'FLOAT', 'IMAGE')
% 'req' (Whether option is required or not), 'def' (option default value if 
% not required) and 'description' (Brief description of option)
function [options] = fabber_get_options(model, method, generic)
    if nargin < 2
        method = '';
    end;
    if nargin < 3
        generic = false;
    end;

    exe = fabber_get_exe(model);
    opt_lines = '';

    if (~isempty(model))
        [status, cmdout] = system([exe ' --model=' model ' --help']);
        if (status == 0);
            opt_lines = [opt_lines cmdout];
        end
    end
    if (~isempty(method))
        [status, cmdout] = system([exe ' --method=' method ' --help']);
        if (status == 0);
            opt_lines = [opt_lines cmdout];
        end
    end
    if (generic)
        [status, cmdout] = system([exe ' --help']);
        if (status == 0);
            opt_lines = [opt_lines cmdout];
        end
    end
    
    options = parse_options(opt_lines);
end

function [options] = parse_options(opt_lines)
    options = [];
    lines = strsplit(opt_lines, '\n');
    option_regex = '--(?<name>.+)\s\[(?<type>.+),(?<req>.+),(?<def>.+)]';
    current_option = [];

    for i = 1:numel(lines);
        line = lines(i);
        [match, names] = regexp(line, option_regex, 'start', 'names');
        if (~isempty(match{1}));
            if (~isempty(current_option));
                options = [options current_option];
            end;
            current_option = names{1};
            current_option.description = [];
            %if (current_option.def == 'NO DEFAULT');
            %    current_option.def = '';
            %else
            %end
        else
            if (~isempty(current_option));
                cur_desc = current_option.description;
                current_option.description = [cur_desc ' ' line];
            end
        end
    end
    if (~isempty(current_option));
        options = [options current_option];
    end;
end
