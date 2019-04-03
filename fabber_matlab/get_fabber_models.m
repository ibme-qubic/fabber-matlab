% get_fabber_models.m
%
% Get all the Fabber executables and models known to the interface. We
% look for Fabber executables in $FSLDIR/bin, $FSLDEVDIR/bin and
% $FABBERDIR/bin. Note that when the same model is found in multiple
% locations, get_fabber_exe will return the last one found, so the
% highest priority location is $FABBERDIR followed by $FSLDEVDIR
% and $FSLDIR.
%
% Returns an array of structures, each containing the 
% fields 'exe' (name of executable) and 'models' (array of model names)
function [all_models] = get_fabber_models()
    model_num = 1;
    bindirs = {
        [getenv('FSLDIR') '/bin'], 
        [getenv('FSLDEVDIR') '/bin'], 
        [getenv('FABBERDIR') '/bin']
    };
    for d = 1:numel(bindirs);
        bindir = bindirs{d};
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
end
