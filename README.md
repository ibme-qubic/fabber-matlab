Fabber MATLAB interface
=======================

This module contains a thin wrapper between Matlab and Fabber. It works by calling the command
line programs and writing out data sets as temporary Nifti files.

If you already have Fabber installed (e.g. as part of FSL 6.0.1+) this interface should work 
without any further configuration. If you don't have FSL, the best option is to install
a pre-built set of Fabber binaries which contain a selection of model libraries. This package 
can be obtained from:

https://github.com/ibme-qubic/fabber_core/releases

Example usage
-------------

    % Create random 4D dataset and mask
    rundata.data = rand(10, 10, 10, 5);
    rundata.mask = logical(ones(10, 10, 10));

    % Infer using polynomial model
    rundata.model='poly';
    rundata.degree=2;
    rundata.method='vb';
    rundata.noise='white';
    rundata.save_mean=true;

    output = fabber(rundata);

`output` is a structure with a named element for each Fabber output, for example
`output.mean_c0` is the parameter map for the `c0` parameter. `output.modelfit` is
always present and contains the 4D model prediction.

Option specification
--------------------

Many Fabber options contain `-` characters (e.g. `save-mean`), which cannot be used as 
field names in a Matlab structure. This poses a problem, because this is what we use
to pass the `rundata` options in the above example. 

The solution is to substitute underscore characters `_` instead, as we have done 
for the `save-mean` parameter which is specified as `rundata.save_mean`. The Matlab interface
will convert the underscores back into `-` before passing them to Fabber.

Only a couple of core Fabber options contain a `_` normally and these are handled specially.
Model options should never use an underscore in their options.

Additional voxel data
---------------------

Additional data required for some models can be specified in the rundata in the expected
way, for example:

    aif = load_untouch_nii('aif.nii.gz')
    rundata.suppdata = aif.img

Issues/Limitations/ToDo
-----------------------

 - There is currently no provision for matrix-data to be passed directly - for example the dataspec in CEST. As currently, this must be written to a file and the filename passed as the appropriate option.




