%
% Check if the Fabber installation is working properly
%
function [] = fabber_test()
    fprintf('*** Checking Fabber installation ***\n')

    fprintf('\nChecking search paths (first is highest priority):\n')
    fsldir = getenv('FSLDIR');
    fsldevdir = getenv('FSLDEVDIR');
    fabberdir = getenv('FABBERDIR');
    if ~isempty(fsldir)
        disp(['FSLDIR: ' fsldir]);
    end
    if ~isempty(fsldevdir)
        disp(['FSLDEVDIR: ' fsldevdir]);
    end
    if ~isempty(fabberdir)
        disp(['FABBERDIR: ' fabberdir]);
    end
    if isempty(fsldir) & isempty(fsldevdir) & isempty(fabberdir)
        fprintf('WARNING: No search paths found\n')
        fprintf('You need to set at least one of FSLDIR, FSLDEVDIR and FABBERDIR environment variables\n')
    end
    
    fprintf('\nChecking known executables and models:\n')
    groups = fabber_get_models();
    total_models = 0;
    for i = 1:numel(groups);
        group = groups(i);
        disp([group.exe ': Number of models: ' num2str(size(group.models, 2))]);
        total_models = total_models + size(group.models, 2);
    end
    if total_models == 0
        fprintf('WARNING: No models found!\n')
    end

    fprintf('\nChecking options for generic polynomial model:\n')
    options = fabber_get_options('poly');
    if numel(options) == 0;
        fprintf('WARNING: No options found for polynomial model!\n');
    else
        disp(['  - ' options(1).name]);    
    end
    
    fprintf('\nRunning basic fitting check:\n')
    
    % Set up polynomial data
    C = linspace(-100, 100, 20);
    B = linspace(-1, 1, 20);
    A = linspace(-0.1, 0.1, 20);
    T = linspace(0, 10, 50);
    data = zeros(size(A, 2), size(B, 2), size(C, 2), size(T, 2));
    for x=1:size(A, 2)
        for y=1:size(B, 2)
            for z=1:size(C, 2)
                for t=1:size(T, 2)
                    data(x, y, z, t) = A(x)*(T(t)*T(t)) + B(y)*T(t) + C(z);
                end
            end
        end
    end
    
    % Add random Gaussian noise to data
    noise_sigma = 0.8;
    data = data + sqrt(noise_sigma)*randn(size(data));
    
    % Set up Fabber run
    rundata = struct();
    rundata.data = data;
    rundata.mask = ones(size(A, 2), size(B, 2), size(C, 2));
    rundata.model='poly';
    rundata.degree = 2;
    rundata.method='vb';
    rundata.noise='white';
    rundata.save_mean = true;
    rundata.save_std = false;
    rundata.save_mvn = false;
    rundata.save_model_fit = true;

    % Run Fabber
    output = fabber(rundata);
    
    % Display sample model fit
    plot(T, squeeze(output.modelfit.img(3, 3, 3, :)), T, squeeze(data(3, 3, 3, :)));
    disp('DONE - check plot to ensure fitting looks accurate')
end

