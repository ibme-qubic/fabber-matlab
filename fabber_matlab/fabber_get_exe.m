% fabber_get_exe.m
%
% Get the executable to use when running a specified model
function [exe] = fabber_get_exe(model)
    all_models = fabber_get_models();
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
