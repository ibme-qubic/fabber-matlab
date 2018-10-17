Fabber MATLAB interface
=======================

This module contains a thin wrapper between Matlab and Fabber. Example usage would be:

    % Create random 4D dataset and mask
    data = rand(10, 10, 10, 5);
    mask = logical(ones(10, 10, 10));

    rundata.model='poly';
    rundata.degree=2;
    rundata.method='vb';
    rundata.noise='white';
    rundata.save_mean='';

    output = fabber(data, mask, rundata);

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

No core Fabber options contain a `_` normally, so this will usually work. However, some
Fabber models may have options which genuinely contain the `_` character. To solve this
problem, the Matlab module checks known model options, and if an option is specified which 
matches one of these, it will be used directly without substitution of underscores. 

*So, just replace `-` with `_` in your option names and everything should work...*

Additional voxel data
---------------------

Additional data required for some models can be specified in the rundata in the expected
way, for example:

    aif = load_untouch_nii('aif.nii.gz')
    rundata.suppdata = aif.img

Compiling
---------

The `cmake` build tool is used to compile the C++ Matlab extension.

On Linux/OSX, use `cmake` directly to build the module *CURRENTLY UNTESTED*:

    mkdir build
    cd build
    cmake ..
    make

On windows the `scripts` directory contains two scripts `build.bat` and `build_all.bat`
which should be run from the Visual Studio Tools command prompt, e.g.

    scripts\build.bat x64 release

Issues/Limitations/ToDo
-----------------------

 - There is currently no provision for matrix-data to be passed directly - for example the dataspec in CEST. As currently, this must be written to a file and the filename passed as the appropriate option.




