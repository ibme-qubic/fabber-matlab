% get_fabber_exe.m
%
function [exe] = get_fabber_exe(model)
    all_models = get_fabber_models();
    if (isempty(model))
        model = 'poly';
    end
    for i = 1:numel(all_models);
        model_group = all_models(i);
        for j = 1:numel(model_group.models);
            if (strcmp(model_group.models(j), model) == 1);
                exe = model_group.exe;
            end 
        end
    end
end
