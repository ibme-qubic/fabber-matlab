/**
 * fabber.cpp
 *
 * MATLAB interface to Fabber. 
 */

#include "mex.h"

#include <fabber_core/rundata_array.h>
#include <fabber_core/fwdmodel.h>
#include <fabber_core/easylog.h>
#include <fabber_core/setup.h>

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <assert.h>
#include <io.h>  
#include <direct.h>  
#include <fstream>  

using namespace std;

//#define DEBUG
void debug(const char *msg)
{
#ifdef DEBUG 
    mexPrintf("%s\n", msg);
#endif
}

/**
 * Check if dimensions of array are consistent with the 4D data set
 *
 * @param arr Array which must be 3D or 4D
 * @param dims_4d An array of 4 dimensions from the main data. 
 * @return true if dimesions match, false otherwise
 */
bool dims_match(const mxArray *arr, const mwSize *dims_4d)
{
    mwSize arr_ndims = mxGetNumberOfDimensions(arr);
    const mwSize *arr_dims = mxGetDimensions(arr);
    int ndims = 4;
    if (arr_ndims < ndims) ndims = arr_ndims;
    for (int i=0; i<ndims; i++) {
        if (dims_4d[i] != arr_dims[i]) { 
            return false;
        }
    }
    return true;
}

/**
 * Ensure dimensions of array are consistent with the 4D data set
 *
 * @param arr Array which must be 3D or 4D
 * @param dims_4d An array of 4 dimensions from the main data. 
 * @param name Name for use in error output
 */
void check_dims(const mxArray *arr, const mwSize *dims_4d, const char *name)
{
    if (!dims_match(arr, dims_4d)) {
        mexPrintf("Data item: %s\n", name);
        mexErrMsgIdAndTxt("Fabber:run:check_dims",
                          "Dimensions of above item are not compatible with main data");
    }
}

/** 
 * Make sure input and output arrays are as we expect
 *
 * Checks for three inputs, main data, mask and rundata
 * Checks for single output or none (to use default 'ans' return value)
 * Checks main data is 4D, mask is 3D and their dimensions are compatible
 * Checks data is double type and mask is logical type
 * Checks rundata is a struct with one element
 *
 * @return On success, array of main data dimensions which will have length 4
 */
const mwSize *validate_input(int nlhs, mxArray *plhs[],
                             int nrhs, const mxArray *prhs[])
{
    /* Check of total number of inputs and outputs */
    if(nrhs != 3) {
        mexErrMsgIdAndTxt("fabber:wrongNumberInputs", "Three input arguments expected");
    }
    
    if(nlhs > 1) {
        mexErrMsgIdAndTxt("fabber:wrongNumberOutputs", "Only one output expected.");
    }
    
    /* Checks on the main data input */
    if( !mxIsDouble(prhs[0]) || mxIsComplex(prhs[0])) {
         mexErrMsgIdAndTxt("fabber:dataNotRealDouble",
                           "Input data must be type real double");
    }
    
    mwSize ndims = mxGetNumberOfDimensions(prhs[0]);
    if (ndims != 4) {
         mexErrMsgIdAndTxt("Fabber:dataNot4D", "Input data must be 4D");
    }
    const mwSize *dims_4d = mxGetDimensions(prhs[0]);
    
    /* Checks on the mask input */
    if (!mxIsLogical(prhs[1])) {
        mexErrMsgIdAndTxt("Fabber:maskNotLogical", "Mask data must be logical");
    }
    
    mwSize mask_ndims = mxGetNumberOfDimensions(prhs[1]);
    if (mask_ndims != 3) {
         mexErrMsgIdAndTxt("Fabber:maskNot3D", "Mask data must be 3D");
    }
    const mwSize *mask_dims = mxGetDimensions(prhs[1]);
    check_dims(prhs[1], dims_4d, "mask");
    
    /* Checks on the rundata input */
    if(!mxIsStruct(prhs[2])) {
         mexErrMsgIdAndTxt("Fabber:rundataNotStruct",
                           "Third argument (rundata) must be a struct");
    }
    
    mwSize NStructElems = mxGetNumberOfElements(prhs[2]);
    if (NStructElems != 1) {
         mexErrMsgIdAndTxt("Fabber:rundataMultiValued",
                           "Third argument (rundata) must contain only one struct");
    }  
    
    return dims_4d;
}

/** 
 * Determine if an array contains a single value
 */
bool single_value(mxArray *arr) 
{
	return (mxGetNumberOfElements(arr) == 1);
}

/**
 * Interpret an array as containing a string and add it as an option value
 */
void add_string(FabberRunDataArray *fab, const char *key, const mxArray *arr)
{
    char *fvalue = mxArrayToString(arr);
    //mexPrintf("Rundata: field %s=%s - char\n", key, fvalue);
    fab->Set(key, fvalue);
    mxFree(fvalue);
}

/**
 * Interpret an array as containing a number and add it as an option value
 */
void add_numeric(FabberRunDataArray *fab, const char *key, const mxArray *arr)
{
    void *data = mxGetData(arr);
    if (mxIsDouble(arr)) {
        double val = ((double *)data)[0];
        //mexPrintf("Rundata: field %s=%f - double\n", key, val);
        fab->Set(key, val);
    }
    else if (mxIsSingle(arr)) {
        float val = ((float *)data)[0];
        //mexPrintf("Rundata: field %s=%f - single\n", key, val);
        fab->Set(key, val);
    }
//    else {
//        mexErrMsgIdAndTxt("Fabber:add_numeric", "Integer options not yet implemented - use a real instead");
//    }
}

char *mkdtemp(char *tmpl)
{
	if (!*_mktemp(tmpl) || _mkdir(tmpl))
		return NULL;
	return tmpl;
}

string matrix_to_tempfile(const char *key, double *data, int nx, int ny)
{
    char *tmpl = strdup("fabmexXXXXXX");
    char* tempdir = mkdtemp(tmpl);
    if (!tempdir) {
        free(tmpl);
        mexErrMsgIdAndTxt("Fabber:matrix_to_tempfile", "Failed to create temporary file");
    }

    string filename = tempdir;
    free(tmpl);
    filename += "/";
    filename += key;
    filename += ".mat";
    mexPrintf("Temp filename is %s\n", filename.c_str());

    // Write matrix data as tab-separated making sure we preserve the MATLAB data ordering
    ofstream matrix_file;
	matrix_file.open(filename);
    for (int x=0; x<nx; x++) {
        for (int y=0; y<ny; y++) {
            matrix_file << data[y*nx + x] << '\t';
        }
        matrix_file << endl;
    }
    matrix_file.close();
    return filename;
}

/**
 * Interpret an array as containing a matrix to be passed as a temporary ASCII file
 */
void add_matrixfile(FabberRunDataArray *fab, const char *key, const mxArray *arr)
{
    mwSize arr_ndims = mxGetNumberOfDimensions(arr);
    unsigned int data_size = 1;
    if (arr_ndims != 2) {
        mexPrintf("Rundata item: %s\n", key);
        mexErrMsgIdAndTxt("Fabber:add_matrixfile", "Matrix options must be 2 dimensional");
    }
    const mwSize *dims_2d = mxGetDimensions(arr);
    void *data = mxGetData(arr);

    if (mxIsDouble(arr)) {
        double *vals = (double *)data;
        fab->Set(key, matrix_to_tempfile(key, vals, dims_2d[0], dims_2d[1]));
    }
    else if (mxIsSingle(arr)) {
        float *vals = (float *)data;
        mexErrMsgIdAndTxt("Fabber:add_matrixfile", "single precision matrices not yet implemented - use a real instead");
    }
    else {
        mexErrMsgIdAndTxt("Fabber:add_matrixfile", "Integer matrices not yet implemented - use a real instead");
    }
}

/**
 * Add Matlab array as named voxel data
 *
 * To do this successfully the array must be consistent with the main data dimensions. Note that
 * FabberRunDataArray using column-major order, as does Matlab, so we can directly copy
 * the Matlab data array.
 */
void add_data(FabberRunDataArray *fab, const char *key, const mxArray *arr, const mwSize *dims_4d)
{
    // Make sure data is double type and matches dimensions of main data
    check_dims(arr, dims_4d, key);
    if (!mxIsDouble(arr)) {
        mexPrintf("Data: %s\n", key);
        mexErrMsgIdAndTxt("Fabber:dataNotDouble", "Voxel data above must by type double");
    }
    double *data = (double *)mxGetData(arr);
    //mexPrintf("Rundata: field %s=%f - double\n", key, data[0]);
    size_t len = mxGetNumberOfElements(arr);
    
    // Find out the data size, i.e. how many points in the 4th dimension
    mwSize arr_ndims = mxGetNumberOfDimensions(arr);
    unsigned int data_size = 1;
    if (arr_ndims == 4) {
        const mwSize *arr_dims = mxGetDimensions(arr);
        data_size = arr_dims[3];
    }
    
    // Copy data to float buffer;
    vector<float> fdata(len);
    for (int i=0; i<len; i++) {
        fdata[i] = data[i];
    }
    
    fab->SetVoxelDataArray(key, data_size, &fdata[0]);
}

/**
 * Get a list of model option names.
 *
 * @see real_option_name
 */
vector<OptionSpec> get_model_options(string model_name)
{
    auto_ptr<FwdModel> model(FwdModel::NewFromName(model_name));
    EasyLog log;
    model->SetLogger(&log); // We ignore the log but this stops it going to cerr
    vector<OptionSpec> options;
    model->GetOptions(options);
    return options; 
}

/**
 * Map the option name from Matlab onto the real Fabber option name
 *
 * We need this because many option names contain '-' which is illegal
 * in Matlab. We adopt the method of interpreting '_' as '-' however this
 * will break options that really do contain '_'. So the rule is:
 * if the option is found in model_options, just keep it as is. If it
 * is not found and contains '_', replace '_' with '-'.
 *
 * Core fabber options should never contain '_'
 */
string real_option_name(string option, vector<OptionSpec> &model_options)
{
    for (vector<OptionSpec>::iterator iter = model_options.begin(); iter != model_options.end(); ++iter)
    {
        if (option == iter->name) {
            return option;
        }
    }

    // Option not found in model - Replace underscore with '-'
    std::replace(option.begin(), option.end(), '_', '-');
    return option;
}

/**
 * Determine if an option is supposed to contain a matrix (NOT voxel data)
 *
 * These options are normally passed as filenames of ASCII matrix files but
 * we can sneakily pass them as native MATLAB matrices
 */
bool is_matrix_option(string option, vector<OptionSpec> &model_options)
{ 
    for (vector<OptionSpec>::iterator iter = model_options.begin(); iter != model_options.end(); ++iter)
    {
        if (option == iter->name) {
            return (iter->type == OPT_MATRIX);
        }
    }

    return false;
}

/** 
 * Convert third input, which must be a MATLAB struct, into a 
 * FabberRunData object
 */
void set_rundata(FabberRunDataArray *fab, const mxArray *rd_str, const mwSize *dims_4d)
{
    int nfields = mxGetNumberOfFields(rd_str); 
    
    // Get model option names - need to first know the model name
    // We use the list of model names to map MATLAB struct field
    // names onto Fabber options which can contain '-' characters
    vector<OptionSpec> model_options;
    for(int i=0; i<nfields; i++) {
        const char *field_name = mxGetFieldNameByNumber(rd_str, i);
        if (strcmp(field_name, "model") == 0) {
            mxArray *field = mxGetFieldByNumber(rd_str, 0, i);
            char *model = mxArrayToString(field);
            model_options = get_model_options(model);
        }
    }
    
    // Now go through each field and determine what kind of option it is
    for(int i=0; i<nfields; i++) {
        const char *field_name = mxGetFieldNameByNumber(rd_str, i);
        string name = real_option_name(field_name, model_options);

        // Loadmodels option already dealt with in load_models()
        if (name == "loadmodels") continue;

        mxArray *field = mxGetFieldByNumber(rd_str, 0, i);
        if (field == NULL) {
            mexPrintf("Rundata: field %d(%s)\n", i+1, name.c_str());
            mexErrMsgIdAndTxt("Fabber:emptyField", "Above field is empty!");
        }
        //mexPrintf("Rundata: field %s\n", fname);
        if(mxIsChar(field)) {
            add_string(fab, name.c_str(), field);
        }
        else if (mxIsNumeric(field)) {
            if (single_value(field)) {
                add_numeric(fab, name.c_str(), field);
            }
            else if (is_matrix_option(name, model_options)) {
                // This is a matrix input that Fabber expects as an ASCII matrix file
                mexPrintf("Matix option: %s\n", name);
                add_matrixfile(fab, name.c_str(), field);
            }
            else {
                add_data(fab, name.c_str(), field, dims_4d);
            }
        }
        else {
            mexPrintf("Rundata: option %d(%s)\n", i+1, name.c_str());
            mexErrMsgIdAndTxt("Fabber:invalidOptionType", "Above option must contain string or numeric data");
        }
    }
}

/** 
 * Set the extent of the voxel co-ordinates and the mask which determines
 * which are to be included in the calculation
 */
void set_extent(FabberRunDataArray *fab, const mwSize *dims_4d, const mxArray *mask)
{
    int nx = dims_4d[0];
    int ny = dims_4d[1];
    int nz = dims_4d[2];
    int len = nx*ny*nz;
        
    vector<int> maskdata(len);
    assert(len == mxGetNumberOfElements(mask));
    
    mxLogical *maskdata_orig = mxGetLogicals(mask);
    for (int i=0; i<len; i++) maskdata[i] = (int)maskdata_orig[i];
    
    fab->SetExtent(nx, ny, nz, &maskdata[0]);
}

/**
 * Get the expected data item outputs which depend on the model
 * parameters, and what the user has asked to save
 */
vector<string> get_outputs(FabberRunDataArray *fab)
{
    auto_ptr<FwdModel> model(FwdModel::NewFromName(fab->GetString("model")));
    vector<string> outputs;
    EasyLog log;
    model->SetLogger(&log); // We ignore the log but this stops it going to cerr
    model->Initialize(*fab);

    // Parameter mean/std
    vector<Parameter> params;
    model->GetParameters(*fab, params);
    vector<Parameter>::iterator iter;
    for (iter = params.begin(); iter != params.end(); ++iter)
    {
        if (fab->GetBool("save-mean")) {
            outputs.push_back(string("mean_") + iter->name);
        }
        if (fab->GetBool("save-std")) {
            outputs.push_back(string("stdev_") + iter->name);
        } 
        if (fab->GetBool("save-zstat")) {
            outputs.push_back(string("zstat_") + iter->name);
        }
    }

    // 'Extra' outputs which may be provided by the model
    if (fab->GetBool("save-model-extras")) {
        vector<string> extra_outputs;
        model->GetOutputs(extra_outputs);
        vector<string>::iterator siter;
        for (siter = extra_outputs.begin(); siter != extra_outputs.end(); ++siter) {
            outputs.push_back(*siter);
        }
    }
    
    // Additional outputs not linked to parameters
    if (fab->GetBool("save-model-fit")) {
        outputs.push_back("modelfit");
    }
    if (fab->GetBool("save-residuals")) {
        outputs.push_back("residuals");
    }
    if (fab->GetBool("save-free-energy")) {
        outputs.push_back("freeEnergy"); // FIXME
    }
    if (fab->GetBool("save-noise_mean")) {
        outputs.push_back("noise_means");
    }
    if (fab->GetBool("save-noise_std")) {
        outputs.push_back("noise_stdevs");
    }
    if (fab->GetBool("save-mvn")) {
        outputs.push_back("finalMVN");
    }

    // Make sure something is output
    if (outputs.size() == 0) {
        fab->SetBool("save-model-fit");
        outputs.push_back("modelfit");
    }

    return outputs;
}

/**
 * Run Fabber
 *
 * Not using progress check currently
 */
string run(FabberRunDataArray *fab)
{
    EasyLog log;
    fab->SetLogger(&log);
    stringstream logstr;
    log.StartLog(logstr);
    if (false)
    {
        //CallbackProgressCheck prog(progress_cb);
        //rundata->Run(&prog);
    }
    else
    {
        fab->Run();
    }
    log.ReissueWarnings();
    return logstr.str();
}

/**
 * Save the Fabber data outputs to the Matlab return structure
 */
void save_output(FabberRunDataArray *fab, mxArray *plhs[], vector<string> output_items, const mwSize *dims_4d, const char *log)
{
    // Need to construct a char ** array of item names
    vector<const char *> item_names;
    for (int i=0; i<output_items.size(); i++) {
        item_names.push_back(output_items[i].c_str());
    }
    item_names.push_back("log");

    mwSize output_struct_dims[2] = {1,1};
    plhs[0] = mxCreateStructArray(2, output_struct_dims, item_names.size(), &item_names[0]);

    for (int i=0; i<output_items.size(); i++) {
        string name = output_items[i];
        debug("Saving output item");
        debug(name.c_str());

        // Construct the dimensions array depending on the data size
        int data_size = fab->GetVoxelDataSize(name);
        vector<mwSize> dims;
        for (int d=0; d<3; d++) {
            dims.push_back(dims_4d[d]);
        }

        mxArray *arr;
        if (data_size > 1) {
            dims.push_back(data_size);
            arr = mxCreateNumericArray(4, &dims[0], mxSINGLE_CLASS, mxREAL);
        }
        else {
            arr = mxCreateNumericArray(3, &dims[0], mxSINGLE_CLASS, mxREAL);
        }
        float *outdata = (float *)mxGetData(arr);
        fab->GetVoxelDataArray(name, outdata);
        mxSetField(plhs[0], 0, name.c_str(), arr);
    }

    // Set the log in the output struct
    debug("Saving output log");
    mxArray *log_arr = mxCreateString(log);  
    mxSetField(plhs[0], 0, "log", log_arr);
}

void load_models(const mxArray *rundata_struc)
{
    int nfields = mxGetNumberOfFields(rundata_struc); 
    
    // Get loadmodels options. Note that we need to do this before our
    // generic parsing of the rundata struct because the model libraries
    // need to be loaded before then.
    string loadmodels;
    for(int i=0; i<nfields; i++) {
        const char *field_name = mxGetFieldNameByNumber(rundata_struc, i);
        if (strcmp(field_name, "loadmodels") == 0) {
            mxArray *field = mxGetFieldByNumber(rundata_struc, 0, i);
            loadmodels = mxArrayToString(field);
        }
    }

    // loadmodels is delimited by semicolons
    std::string delimiter = ";";

    size_t pos = 0;
    debug("Loading model libraries");
    while ((pos = loadmodels.find(delimiter)) != std::string::npos) {
        std::string modellib = loadmodels.substr(0, pos);
        try {
            debug(modellib.c_str());
            FwdModel::LoadFromDynamicLibrary(modellib);
        }
        catch (...) {
            mexPrintf("WARNING: failed to load model library %s\n", modellib.c_str());
        }
        loadmodels.erase(0, pos + delimiter.length());
    }
}

/**
 * Entry point to fabber function 
 */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    const mwSize *dims_4d = validate_input(nlhs, plhs, nrhs, prhs);
    
    try {
        FabberSetup::SetupDefaults();
        FabberRunDataArray *fab = new FabberRunDataArray(false);
        debug("Created fab\n");

        // Set the data extent and mask
        set_extent(fab, dims_4d, prhs[1]);
        debug("Set extent\n");

        // Add the main data
        add_data(fab, "data", prhs[0], dims_4d);
        debug("Added main data\n");

        // Load models from shared libraries
        load_models(prhs[2]);
        debug("Loaded models\n");

        // Set up the options from the rundata argument
        set_rundata(fab, prhs[2], dims_4d);
        debug("Set rundata\n");

        // Figure out what output data we're expecting 
        vector<string> outputs = get_outputs(fab);
        debug("Got outputs\n");

        // Run fabber
        string log = run(fab);
        debug(log.c_str());

        // Save output data items 
        save_output(fab, plhs, outputs, dims_4d, log.c_str());
        debug("saved outputs\n");        
    }
    catch (const FabberError &e)
    {
        mexErrMsgIdAndTxt("Fabber:fabberError", e.what());
    }
    catch (const exception &e)
    {
        mexErrMsgIdAndTxt("Fabber:stlException", e.what());
    }
    catch (NEWMAT::Exception &e)
    {
        mexErrMsgIdAndTxt("Fabber:newmatException", e.what());
    }
    catch (...)
    {
        mexErrMsgIdAndTxt("Fabber:otherException", "Unrecognized exception");
    }
}
