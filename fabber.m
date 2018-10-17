function [output] = fabber(data, mask, rundata)

% fabber.m
%
% Simple MATLAB wrapper to handle some limitations of the native wrapper - it is 
% much easier to write this bit in MATLAB rather than build it into the C++ code
%

lib_subdir = '/bin/';
name_prefix = 'fabber_models_';
name_suffix = '.dll';

% The data must be double type
d = double(data);

% The mask must be logical type
m = logical(mask);

% Load model libraries by passing them via the rundata as a comma separated list
fsldir = getenv('FSLDIR');
rundata.loadmodels = '';
fabber_libs = dir([fsldir lib_subdir name_prefix '*' name_suffix]);
for l = 1:length(fabber_libs)
    file_name = fabber_libs(l).name;
    if (length(rundata.loadmodels) > 0)
      rundata.loadmodels = [rundata.loadmodels ',']
    end
    rundata.loadmodels = [rundata.loadmodels fsldir lib_subdir file_name];
end

output = fabber_wrapper(d, m, rundata);
end
