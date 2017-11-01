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

using namespace std;

//#define DEBUG
void debug(const char *msg)
{
#ifdef DEBUG 
    mexPrintf(msg);
#endif
}

/**
 * Check if dimensions of array are consistent with the 4D data set
 *
 * @param arr Array which must be 3D or 4D
 * @param dims_4d An array of 4 dimensions from the main data. 
 * @param name Name for use in error output
 */
void dims_match(const mxArray *arr, const mwSize *dims_4d, const char *name)
{
    mwSize arr_ndims = mxGetNumberOfDimensions(arr);
    const mwSize *arr_dims = mxGetDimensions(arr);
    int ndims = 4;
    if (arr_ndims < ndims) ndims = arr_ndims;
    for (int i=0; i<ndims; i++) {
        if (dims_4d[i] != arr_dims[i]) { 
            mexPrintf("Data item: %s\n", name);
            mexErrMsgIdAndTxt("Fabber:run:dims_match",
                              "Dimensions of above item are not compatible with main data");
        }
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
    dims_match(prhs[1], dims_4d, "mask");
    
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
    debug("Rundata: add_string\n");
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
    else {
        // FIXME does not seem to work
        mexErrMsgIdAndTxt("Fabber:intOptions", "Integer options not yet working - use a real instead");
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
    dims_match(arr, dims_4d, key);
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
vector<string> model_option_names(string model_name)
{
    auto_ptr<FwdModel> model(FwdModel::NewFromName(model_name));
    vector<string> option_names;
    EasyLog log;
    model->SetLogger(&log); // We ignore the log but this stops it going to cerr
    vector<OptionSpec> options;
    model->GetOptions(options);
    for (vector<OptionSpec>::iterator iter = options.begin(); iter != options.end(); ++iter)
    {
        option_names.push_back(iter->name);
    }

    return option_names; 
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
string real_option_name(string option, vector<string> &model_options)
{
    //mexPrintf("real_option_name: in %s\n", option);
    if (std::find(model_options.begin(), model_options.end(), option) == model_options.end())
    {
        // Option not found in model - Replace underscore with '-'
        std::replace(option.begin(), option.end(), '_', '-');
        //mexPrintf("real_option_name: out %s\n", option);
    }
    return option;
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
    vector<string> model_options;
    for(int i=0; i<nfields; i++) {
        const char *field_name = mxGetFieldNameByNumber(rd_str, i);
        if (strcmp(field_name, "model") == 0) {
            model_options = model_option_names("poly");
        }
    }
    
    // Now go through each field and determine what kind of option it is
    for(int i=0; i<nfields; i++) {
        //mexPrintf("Rundata: field %d\n", i+1);
        string name = real_option_name(mxGetFieldNameByNumber(rd_str, i), model_options);
        mxArray *field = mxGetFieldByNumber(rd_str, 0, i);
        if(field == NULL) {
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
    vector<Parameter> params;
    model->GetParameters(*fab, params);
    vector<Parameter>::iterator iter;
    for (iter = params.begin(); iter != params.end(); ++iter)
    {
        //mexPrintf("Param: %s", iter->name);
        if (fab->GetBool("save-mean")) {
            outputs.push_back(string("mean_") + iter->name);
        }
        if (fab->GetBool("save-std")) {
            outputs.push_back(string("stdev_") + iter->name);
        }
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
void save_output(FabberRunDataArray *fab, mxArray *plhs[], vector<string> output_items, const mwSize *dims_4d)
{
    // Need to construct a char ** array of item names
    vector<const char *> item_names;
    for (int i=0; i<output_items.size(); i++) {
        //mexPrintf("Outputting %s\n", output_items[i]);
        item_names.push_back(output_items[i].c_str());
    }

    mwSize output_struct_dims[2] = {1,1};
    plhs[0] = mxCreateStructArray(2, output_struct_dims, item_names.size(), &item_names[0]);

    for (int i=0; i<output_items.size(); i++) {
        string name = output_items[i];

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
        //mexPrintf("Created output array for %s\n", name);
        fab->GetVoxelDataArray(name, outdata);
        //mexPrintf("Saved %s\n", name);
        mxSetField(plhs[0], 0, name.c_str(), arr);
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

        // Set up the options from the rundata argument
        set_rundata(fab, prhs[2], dims_4d);
        debug("Set rundata\n");

        // Figure out what output data we're expecting 
        vector<string> outputs = get_outputs(fab);
        debug("Got outputs\n");

        // Always output the model fit
        fab->SetBool("save-model-fit");
        outputs.push_back("modelfit");

        // Run fabber
        string log = run(fab);
        debug(log.c_str());

        // Save output data items 
        save_output(fab, plhs, outputs, dims_4d);
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
