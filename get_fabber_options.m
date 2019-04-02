% get_fabber_exe.m
%
function [options] = get_fabber_options(model, method, generic)
    if nargin < 2
        method = '';
    end;
    if nargin < 3
        generic = false;
    end;

    exe = get_fabber_exe(model);
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
            current_option.description = '';
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
end