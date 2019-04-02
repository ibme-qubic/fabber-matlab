% get_fabber_models.m
%
function [all_models] = get_fabber_models()
    model_num = 1;
    bindir = [getenv('FSLDIR') '/bin'];
    files = (dir(bindir));
    for i = 1:numel(files);
        exe = files(i).name;
        if ((strfind(exe, 'fabber_') == 1) & isempty(strfind(exe, '.dll')));
            exe = [bindir '/' exe];
            [status, cmdout] = system([exe ' --listmodels']);
            if (status == 0);
                models = strsplit(cmdout, '\n');
                all_models(model_num).exe = exe;
                all_models(model_num).models = models(~cellfun('isempty', models));
                model_num = model_num + 1;
            end
        end
    end
end
