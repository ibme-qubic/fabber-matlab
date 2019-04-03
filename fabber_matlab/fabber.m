% fabber.m
%
% Run fabber Bayesian model fitting tool
%
% Rundata should be a structure containing as a minimum the following
% fields:
%   - data: Input data either as filename or Matlab array
%   - model: Name of model
%   - method: Name of inference method (e.g. 'vb')
%   - noise: Name of noise model (e.g. 'white')
%
% Plus any further options related to the model or inference.
%
% Output is a structure containing the field 'logfile' and additional
% fields for each output data item, e.g. `mean_c0`, `modelfit`, `finalMVN`.
% Data items are output as Nifti data structures, so to get the image
% data array use the `img` field.
%
% This is based on a stripped down version of the Python command line
% wrapper API. Not all features of this are supported, e.g. listing
% model parameters, getting test data, etc. Some of this may be added
% in the future if there is demand for it.
%
function [output] = fabber(rundata)
    rundata = convert_boolean_options(rundata);
    [rundata, tempfiles] = convert_data_options(rundata);
    rundata.output = tempname();
    
    cl = build_cl(rundata);
    system(cl);
    
    output = read_output(rundata.output);
    
    for i = 1:numel(tempfiles)
        delete(tempfiles{i});
    end
    rmdir(rundata.output, 's');
end

function [cl] = build_cl(rundata)
    % Build command line string from rundata options
    cl = get_fabber_exe(rundata.model);
    args = fieldnames(rundata);
    for i = 1:numel(args)
        arg = args{i};
        val = rundata.(arg);
        if isempty(strfind(arg, 'PSP_'));
            arg = strrep(arg, '_', '-');
        end
        cl = [cl ' --' arg];
        if (~isempty(val));
            cl = [cl '=' num2str(val)];
        end
    end
end

function [rundata] = convert_boolean_options(rundata)
    % Boolean options can be specified as an empty string=true, this is the
    % Fabber expectation, however we also accept logicals (but not 0, 1 because
    % these might be valid option values)
    args = fieldnames(rundata);
    for i = 1:numel(args)
        arg = args{i};
        val = rundata.(arg);
        if islogical(val);
            if val
                rundata.(arg) = '';
            else
                rundata = rmfield(rundata, arg);
            end
        end
    end
end

function [rundata, tempfiles] = convert_data_options(rundata)
    % Handle data options by writing each as a temporary Nifti file
    % Returns the updated rundata (containing filenames, no data arrays)
    % and a list of temp file names so they can be deleted after the
    % run.
    args = fieldnames(rundata);
    tempfiles = {};
    options = get_fabber_options(rundata.model, rundata.method, true);
    for i = 1:numel(args)
        arg = args{i};
        val = rundata.(arg);
        if is_data_option(arg, options);
            if ~ischar(val)
                nii = make_nii(val);
                fname = [tempname() '.nii.gz'];
                save_nii(nii, fname);
                rundata.(arg) = fname;
                tempfiles = [tempfiles, fname];
            end;
        end
    end
end

function [data_option] = is_data_option(key, options)
    % Determine if a named option should contain a data array and
    % therefore be passed as a Nifti file
    data_option = false;
    if strcmp(key, 'data') == 1 data_option = true; end;
    if strcmp(key, 'mask') == 1 data_option = true; end;
    if strcmp(key, 'suppdata') == 1 data_option = true; end;
    if strcmp(key, 'continue-from-mvn') == 1 data_option = true; end;
    if regexp(key, 'PSP_byname.+_image') data_option = true; end;
    for i = 1:numel(options)
        option = options(i);
        if strcmp(key, option.name) == 1
            if strcmp(option.type, 'IMAGE') == 1 data_option = true; end;
            if strcmp(option.type, 'TIMESERIES') == 1 data_option = true; end;
            if strcmp(option.type, 'MVN') == 1 data_option = true; end;
        end
    end
end

function [output] = read_output(outdir)
    % Read output files and populate an output structure
    output.log = fileread([outdir '/logfile']);
       
    alphanum = '[a-zA-Z0-9_]';
    regexes = {
        ['(mean_' alphanum '+)\..+'],
        ['(std_' alphanum '+)\..+'],
        ['(zstat_' alphanum '+)\..+'],
        '(noise_means)\..+',
        '(noise_stdevs)\..+',
        '(finalMVN)\..+',
        '(freeEnergy)\..+',
        '(modelfit)\..+',
    };
    output_files = dir(outdir);
    for i = 1:numel(output_files);
        fname = output_files(i).name;
        for j = 1:numel(regexes);
            regex = regexes(j);
            [match, tokens] = regexp(fname, regex, 'start', 'tokens');
            if (~isempty(match{1}));
                output.(tokens{1}{1}{1}) = load_untouch_nii([outdir '/' fname]);
            end
        end
    end
end
