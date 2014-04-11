/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "USRP_UHD.h"

PREPARE_LOGGING(USRP_UHD_i)

USRP_UHD_i::USRP_UHD_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    USRP_UHD_base(devMgr_ior, id, lbl, sftwrPrfl)
{
    construct();
}

USRP_UHD_i::USRP_UHD_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    USRP_UHD_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
    construct();
}

USRP_UHD_i::USRP_UHD_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    USRP_UHD_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
    construct();
}

USRP_UHD_i::USRP_UHD_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    USRP_UHD_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
    construct();
}

USRP_UHD_i::~USRP_UHD_i()
{
}


/***********************************************************************************************

    Basic functionality:

        The service function is called by the serviceThread object (of type ProcessThread).
        This call happens immediately after the previous call if the return value for
        the previous call was NORMAL.
        If the return value for the previous call was NOOP, then the serviceThread waits
        an amount of time defined in the serviceThread's constructor.
        
    SRI:
        To create a StreamSRI object, use the following code:
                std::string stream_id = "testStream";
                BULKIO::StreamSRI sri = bulkio::sri::create(stream_id);

    Time:
        To create a PrecisionUTCTime object, use the following code:
                BULKIO::PrecisionUTCTime tstamp = bulkio::time::utils::now();

        
    Ports:

        Data is passed to the serviceFunction through the getPacket call (BULKIO only).
        The dataTransfer class is a port-specific class, so each port implementing the
        BULKIO interface will have its own type-specific dataTransfer.

        The argument to the getPacket function is a floating point number that specifies
        the time to wait in seconds. A zero value is non-blocking. A negative value
        is blocking.  Constants have been defined for these values, bulkio::Const::BLOCKING and
        bulkio::Const::NON_BLOCKING.

        Each received dataTransfer is owned by serviceFunction and *MUST* be
        explicitly deallocated.

        To send data using a BULKIO interface, a convenience interface has been added 
        that takes a std::vector as the data input

        NOTE: If you have a BULKIO dataSDDS port, you must manually call 
              "port->updateStats()" to update the port statistics when appropriate.

        Example:
            // this example assumes that the device has two ports:
            //  A provides (input) port of type bulkio::InShortPort called short_in
            //  A uses (output) port of type bulkio::OutFloatPort called float_out
            // The mapping between the port and the class is found
            // in the device base class header file

            bulkio::InShortPort::dataTransfer *tmp = short_in->getPacket(bulkio::Const::BLOCKING);
            if (not tmp) { // No data is available
                return NOOP;
            }

            std::vector<float> outputData;
            outputData.resize(tmp->dataBuffer.size());
            for (unsigned int i=0; i<tmp->dataBuffer.size(); i++) {
                outputData[i] = (float)tmp->dataBuffer[i];
            }

            // NOTE: You must make at least one valid pushSRI call
            if (tmp->sriChanged) {
                float_out->pushSRI(tmp->SRI);
            }
            float_out->pushPacket(outputData, tmp->T, tmp->EOS, tmp->streamID);

            delete tmp; // IMPORTANT: MUST RELEASE THE RECEIVED DATA BLOCK
            return NORMAL;

        If working with complex data (i.e., the "mode" on the SRI is set to
        true), the std::vector passed from/to BulkIO can be typecast to/from
        std::vector< std::complex<dataType> >.  For example, for short data:

            bulkio::InShortPort::dataTransfer *tmp = myInput->getPacket(bulkio::Const::BLOCKING);
            std::vector<std::complex<short> >* intermediate = (std::vector<std::complex<short> >*) &(tmp->dataBuffer);
            // do work here
            std::vector<short>* output = (std::vector<short>*) intermediate;
            myOutput->pushPacket(*output, tmp->T, tmp->EOS, tmp->streamID);

        Interactions with non-BULKIO ports are left up to the device developer's discretion

    Properties:
        
        Properties are accessed directly as member variables. For example, if the
        property name is "baudRate", it may be accessed within member functions as
        "baudRate". Unnamed properties are given a generated name of the form
        "prop_n", where "n" is the ordinal number of the property in the PRF file.
        Property types are mapped to the nearest C++ type, (e.g. "string" becomes
        "std::string"). All generated properties are declared in the base class
        (USRP_UHD_base).
    
        Simple sequence properties are mapped to "std::vector" of the simple type.
        Struct properties, if used, are mapped to C++ structs defined in the
        generated file "struct_props.h". Field names are taken from the name in
        the properties file; if no name is given, a generated name of the form
        "field_n" is used, where "n" is the ordinal number of the field.
        
        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            //  - A boolean called scaleInput
              
            if (scaleInput) {
                dataOut[i] = dataIn[i] * scaleValue;
            } else {
                dataOut[i] = dataIn[i];
            }
            
        Callback methods can be associated with a property so that the methods are
        called each time the property value changes.  This is done by calling 
        addPropertyChangeListener(<property name>, this, &USRP_UHD_i::<callback method>)
        in the constructor.

        Callback methods should take two arguments, both const pointers to the value
        type (e.g., "const float *"), and return void.

        Example:
            // This example makes use of the following Properties:
            //  - A float value called scaleValue
            
        //Add to USRP_UHD.cpp
        USRP_UHD_i::USRP_UHD_i(const char *uuid, const char *label) :
            USRP_UHD_base(uuid, label)
        {
            addPropertyChangeListener("scaleValue", this, &USRP_UHD_i::scaleChanged);
        }

        void USRP_UHD_i::scaleChanged(const float *oldValue, const float *newValue)
        {
            std::cout << "scaleValue changed from" << *oldValue << " to " << *newValue
                      << std::endl;
        }
            
        //Add to USRP_UHD.h
        void scaleChanged(const float* oldValue, const float* newValue);
        
        
************************************************************************************************/

/** RECEIVE THREAD **/
int USRP_UHD_i::serviceFunctionReceive()
{
    // yield to other threads if necessary
    try{
        boost::this_thread::interruption_point();
    } catch(const boost::thread_interrupted& ){
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "INTERRUP REQUESTED, returning NOOP");
        return NOOP;
    }

    if (usrp_device_ptr.get() == NULL)
        return NOOP;

    bool rx_data = false;

    for (size_t tuner_id = 0; tuner_id < usrp_tuners.size(); tuner_id++) {

        if (frontend_tuner_status[tuner_id].tuner_type != "RX_DIGITIZER")
            continue;

        //Check to see if channel is allocated before acquiring lock
        if (getControlAllocationId(tuner_id).empty()){
            continue;
        }

        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner " << tuner_id);

        //Check to make sure channel is allocated still
        if (getControlAllocationId(tuner_id).empty()){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << "  (not allocated anymore)");
            continue;
        }

        //Check to see if channel output is enabled
        if (!frontend_tuner_status[tuner_id].enabled){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << "  (not enabled)");
            continue;
        }

        if(usrpReceive(tuner_id)){
            rx_data = true;
        } else {
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << " (no data received)");
            continue;
        }

        if (usrp_tuners[tuner_id].buffer_capacity > usrp_tuners[tuner_id].buffer_size){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << " (buffer not full yet)");
            continue;
        }

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "  pushing buffer of " << usrp_tuners[tuner_id].buffer_size/2 << " samples");

        // get stream id (creates one if not already created for this tuner)
        std::string stream_id = getStreamId(tuner_id);

        // Send updated SRI
        if (usrp_tuners[tuner_id].update_sri){
            BULKIO::StreamSRI sri = create(stream_id, frontend_tuner_status[tuner_id]);
            sri.mode = 1; // complex
            dataShort_out->pushSRI(sri);
            usrp_tuners[tuner_id].update_sri = false;
        }

        // Pushing Data
        dataShort_out->pushPacket(usrp_tuners[tuner_id].output_buffer, usrp_tuners[tuner_id].output_buffer_time, false, stream_id);
        usrp_tuners[tuner_id].buffer_size = 0;
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << " (end of loop)");
    }

    if(rx_data)
        return NORMAL;
    return NOOP;
}

/** TRANSMIT THREAD **/
int USRP_UHD_i::serviceFunctionTransmit(){

    // yield to other threads if necessary
    try{
        boost::this_thread::interruption_point();
    } catch(const boost::thread_interrupted& ){
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "INTERRUP REQUESTED, returning NOOP");
        return NOOP;
    }

    bool ret = transmitHelper(dataShortTX_in);
    ret = ret || transmitHelper(dataFloatTX_in);
    if(ret)
        return NORMAL;
    return NOOP;

}

void USRP_UHD_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);
    //USRP_UHD_base::start();

    // Create additional transmit thread
    try {

        {
            exclusive_lock lock(receive_service_thread_lock);
            if (receive_service_thread == NULL) {
                dataShortTX_in->unblock();
                dataFloatTX_in->unblock();
                //receive_service_thread = new ReceiveProcessThread<USRP_UHD_i> (this, 0.1);
                receive_service_thread = new ReceiveProcessThread<USRP_UHD_i> (this, 0.001);
                receive_service_thread->start();
            }
        }
        {
            exclusive_lock lock(transmit_service_thread_lock);
            if (transmit_service_thread == NULL) {
                dataShortTX_in->unblock();
                dataFloatTX_in->unblock();
                //transmit_service_thread = new TransmitProcessThread<USRP_UHD_i> (this, 0.1);
                transmit_service_thread = new TransmitProcessThread<USRP_UHD_i> (this, 0.001);
                transmit_service_thread->start();
            }
        }

        //serviceThread->updateDelay(0.001);
        //receive_service_thread->updateDelay(0.001);
        //transmit_service_thread->updateDelay(0.001);

    } catch (...) {
        stop();
        throw;
    }

    if (!Resource_impl::started()) {
        Resource_impl::start();
    }
}

void USRP_UHD_i::stop() throw (CORBA::SystemException, CF::Resource::StopError) {
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);
    //USRP_UHD_base::stop();

    {
        exclusive_lock lock(receive_service_thread_lock);
        // release the child thread (if it exists)
        if (receive_service_thread != 0) {
            dataShortTX_in->block();
            dataShortTX_in->block();
            if (!receive_service_thread->release(2)) {
                throw CF::Resource::StopError(CF::CF_NOTSET,"Receive processing thread did not die");
            }
            delete receive_service_thread;
            receive_service_thread = 0;
        }
    }

    {
        exclusive_lock lock(transmit_service_thread_lock);
        // release the child thread (if it exists)
        if (transmit_service_thread != 0) {
            dataShortTX_in->block();
            dataShortTX_in->block();
            if (!transmit_service_thread->release(2)) {
                throw CF::Resource::StopError(CF::CF_NOTSET,"Transmit processing thread did not die");
            }
            delete transmit_service_thread;
            transmit_service_thread = 0;
        }
    }

    // iterate through tuners to disable any enabled tuners
    for (size_t tuner_id = 0; tuner_id < usrp_tuners.size(); tuner_id++) {
        //LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "disabling output for for tuner " << tuner_id);
        deviceDisable(tuner_id);
    }

    if (Resource_impl::started()) {
        Resource_impl::stop();
    }
}

void USRP_UHD_i::construct()
{
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);
    receive_service_thread = NULL;
    transmit_service_thread = NULL;

    // set some default values that should get overwritten by correct values
    device_gain_global = 0.0;
    device_reference_source_global = "INTERNAL";
    device_group_id_global = "USRP_GROUP_ID_NOT_SET";
    rx_rfinfo_pkt.rf_flow_id = "USRP_RX_FLOW_ID_NOT_SET";
    tx_rfinfo_pkt.rf_flow_id = "USRP_TX_FLOW_ID_NOT_SET";

    USRP_UHD_base::construct();
    /***********************************************************************************
     this function is invoked in the constructor
    ***********************************************************************************/

    addPropertyChangeListener("device_ip_address", this, &USRP_UHD_i::deviceIpAddressChanged);
    addPropertyChangeListener("device_gain_global", this, &USRP_UHD_i::deviceGainChanged);
    addPropertyChangeListener("device_group_id_global", this, &USRP_UHD_i::deviceGroupIdChanged);
    addPropertyChangeListener("update_available_devices", this, &USRP_UHD_i::updateAvailableDevicesChanged);
    addPropertyChangeListener("device_reference_source_global", this, &USRP_UHD_i::deviceReferenceSourceChanged);

    if(update_available_devices){
        update_available_devices = false;
        updateAvailableDevices();
    }

    /** As of the REDHAWK 1.8.3 release, device are not started automatically by the node. Therefore
     *  the device must start itself. */
    start();
}
/*************************************************************
Functions supporting tuning allocation
*************************************************************/
void USRP_UHD_i::deviceEnable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to set the 'enabled' member of fts to indicate that tuner as enabled
    ************************************************************/
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);
    //if (tuner_id >= sim_tuners.size())
    //    throw FRONTEND::BadParameterException("deviceEnable: INVALID TUNER ID");

    size_t usrp_channel_number=-1;
    try {
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
        exclusive_lock lock(prop_lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

        usrp_channel_number = device_channels[tuner_id].chan_num;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
    } catch (...) {
        LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
        throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
    }

    // Start Streaming Now
    interrupt(tuner_id);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
    exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

    //bool retval =
    usrpEnable(tuner_id, usrp_channel_number);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);
    //return retval;
}
void USRP_UHD_i::deviceDisable(frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    Make sure to reset the 'enabled' member of fts to indicate that tuner as disabled
    ************************************************************/
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);
    //if (tuner_id >= sim_tuners.size())
    //    throw FRONTEND::BadParameterException("deviceDisable: INVALID TUNER ID");

    size_t usrp_channel_number=-1;
    try {
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " waiting for prop_lock");
        exclusive_lock lock(prop_lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " acquired prop_lock");

        usrp_channel_number = device_channels[tuner_id].chan_num;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " releasing prop_lock");
    } catch (...) {
        LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
        throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
    }

    // Stop Streaming Now
    interrupt(tuner_id);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " waiting for tuner lock for tuner_id " << tuner_id);
    exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " acquired tuner lock for tuner_id " << tuner_id);

    //bool retval =
    usrpDisable(tuner_id, usrp_channel_number);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " releasing tuner lock for tuner_id " << tuner_id);
    //return retval;
}
bool USRP_UHD_i::deviceSetTuning(const frontend::frontend_tuner_allocation_struct &request, frontend_tuner_status_struct_struct &fts, size_t tuner_id){
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    return true if the tuning succeeded, and false if it failed
    ************************************************************/
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);
    /************************************************************
    modify this->tuner_allocation_ids

    This data structure is a vector of tuner_status structures
    that is referenced by the base class to determine whether
    or not tuners are available
    ************************************************************/
    //if (tuner_id >= simulated_channel_seq.size())
    //    throw FRONTEND::BadParameterException("deviceSetTuning: INVALID TUNER ID");
    //if(tuner_type != simulated_channel_seq[tuner_id].tuner_type)
    //    throw FRONTEND::BadParameterException("deviceSetTuning: INVALID TUNER TYPE");

    size_t usrp_channel_number = -1;
    double opt_sr = request.sample_rate;
    double opt_bw = request.bandwidth;
    { // scope for prop_lock
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "waiting for prop_lock");
        exclusive_lock lock(prop_lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "acquired prop_lock");

        if (frontend::compareHz(request.center_frequency,device_channels[tuner_id].freq_min) < 0 ||
                frontend::compareHz(request.center_frequency,device_channels[tuner_id].freq_max) > 0 ){
            LOG_INFO(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID CENTER FREQUENCY (" << request.center_frequency <<")" );
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "releasing prop_lock");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__) + " : INVALID CENTER FREQUENCY").c_str());
        }

        if (frontend::compareHz(request.bandwidth,0.0) < 0 ||
                frontend::compareHz(request.bandwidth,device_channels[tuner_id].bandwidth_max) > 0 ){
            LOG_INFO(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID BANDWIDTH (" << request.bandwidth <<")" );
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "releasing prop_lock");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__) + " : INVALID BANDWIDTH").c_str());
        }

        if (frontend::compareHz(request.sample_rate,0.0) < 0 ||
                frontend::compareHz(request.sample_rate,device_channels[tuner_id].rate_max) > 0 ){
            LOG_INFO(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID SAMPLE RATE (" << request.sample_rate <<")" );
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "releasing prop_lock");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__) + " : INVALID SAMPLE RATE").c_str());
        }

        opt_sr = optimizeRate(request.sample_rate, tuner_id);
        opt_bw = optimizeBandwidth(request.bandwidth, tuner_id);
        usrp_channel_number = device_channels[tuner_id].chan_num;
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << "releasing prop_lock");
    } // end scope for prop_lock

    if (frontend_tuner_status[tuner_id].tuner_type == "RX_DIGITIZER") {

        interrupt(tuner_id);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

        // configure hw
        usrp_device_ptr->set_rx_freq(request.center_frequency, usrp_channel_number);
        usrp_device_ptr->set_rx_bandwidth(opt_bw, usrp_channel_number);
        usrp_device_ptr->set_rx_rate(opt_sr, usrp_channel_number);

        // update frontend_tuner_status with actual hw values
        frontend_tuner_status[tuner_id].center_frequency = usrp_device_ptr->get_rx_freq(usrp_channel_number);
        frontend_tuner_status[tuner_id].bandwidth = usrp_device_ptr->get_rx_bandwidth(usrp_channel_number);
        frontend_tuner_status[tuner_id].sample_rate = usrp_device_ptr->get_rx_rate(usrp_channel_number);

        // update tolerance
        frontend_tuner_status[tuner_id].bandwidth_tolerance = request.bandwidth_tolerance;
        frontend_tuner_status[tuner_id].sample_rate_tolerance = request.sample_rate_tolerance;

        // creates a stream id if not already created for this tuner
        std::string stream_id = getStreamId(tuner_id);

        // enable multi-out capability for this stream/allocation/connection
        matchAllocationIdToStreamId(request.allocation_id, stream_id, "dataShort_out");

        usrp_tuners[tuner_id].update_sri = true;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);

    } else if (frontend_tuner_status[tuner_id].tuner_type == "TX") {

        interrupt(tuner_id);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

        // configure hw
        usrp_device_ptr->set_tx_freq(request.center_frequency, usrp_channel_number);
        usrp_device_ptr->set_tx_bandwidth(request.bandwidth, usrp_channel_number);
        usrp_device_ptr->set_tx_rate(opt_sr, usrp_channel_number);

        // update frontend_tuner_status with actual hw values
        frontend_tuner_status[tuner_id].center_frequency = usrp_device_ptr->get_tx_freq(usrp_channel_number);
        frontend_tuner_status[tuner_id].bandwidth = usrp_device_ptr->get_tx_bandwidth(usrp_channel_number);
        frontend_tuner_status[tuner_id].sample_rate = usrp_device_ptr->get_tx_rate(usrp_channel_number);

        // update tolerance
        frontend_tuner_status[tuner_id].bandwidth_tolerance = request.bandwidth_tolerance;
        frontend_tuner_status[tuner_id].sample_rate_tolerance = request.sample_rate_tolerance;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);

    } else {
        LOG_ERROR(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!");
        throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__)+" : INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!").c_str());
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateDeviceInfo();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");

    return true;
}
bool USRP_UHD_i::deviceDeleteTuning(frontend_tuner_status_struct_struct &fts, size_t tuner_id) {
    /************************************************************
    modify fts, which corresponds to this->frontend_tuner_status[tuner_id]
    return true if the tune deletion succeeded, and false if it failed
    ************************************************************/
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);
    //if (tuner_id >= usrp_tuners.size())
    //    throw FRONTEND::BadParameterException("deviceDeleteTuning: INVALID TUNER ID");

    interrupt(tuner_id);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
    exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

    usrp_tuners[tuner_id].reset();
    frontend_tuner_status[tuner_id].center_frequency = 0.0;
    frontend_tuner_status[tuner_id].sample_rate = 0.0;
    frontend_tuner_status[tuner_id].bandwidth = 0.0;
    frontend_tuner_status[tuner_id].gain = 0.0;
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);
    return true;
}

void USRP_UHD_i::interrupt(size_t tuner_id){
    if(frontend_tuner_status[tuner_id].tuner_type == "RX_DIGITIZER"){
        exclusive_lock lock(receive_service_thread_lock);
        if(receive_service_thread != NULL){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " interrupting RX thread");
            receive_service_thread->interrupt();
        }
    }
    else if(frontend_tuner_status[tuner_id].tuner_type == "TX"){
        exclusive_lock lock(transmit_service_thread_lock);
        if(transmit_service_thread != NULL){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " interrupting TX thread");
            transmit_service_thread->interrupt();
        }
    }
}

/** A templated service function that is generic between data types. */
template <class IN_PORT_TYPE> bool USRP_UHD_i::transmitHelper(IN_PORT_TYPE *dataIn) {

    if (usrp_device_ptr.get() == NULL)
        return false;

    typename IN_PORT_TYPE::dataTransfer *packet = dataIn->getPacket(0);
    if (packet == NULL)
        return false;

    if (packet->inputQueueFlushed){
        LOG_WARN(USRP_UHD_i,"Input Queue Flushed");
    }

    if (packet->SRI.mode != 1) {
        LOG_ERROR(USRP_UHD_i,"USRP device requires complex data.  Real data type received.");
        delete packet;
        return false;
    }

    for (size_t tuner_id = 0; tuner_id < usrp_tuners.size(); tuner_id++) {
        //Check to see if wideband channel is either not allocated, or the output is not enabled
        if (frontend_tuner_status[tuner_id].tuner_type != "TX")
            continue;

        //Check to see if channel is allocated
        if (getControlAllocationId(tuner_id).empty()){
            continue;
        }

        LOG_INFO(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner " << tuner_id);

        //Check to make sure channel is allocated still
        if (getControlAllocationId(tuner_id).empty()){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << "  (not allocated anymore)");
            continue;
        }

        //Check to see if wideband channel is enabled
        if (!frontend_tuner_status[tuner_id].enabled){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id << "  (not enabled)");
            continue;
        }

        usrpTransmit(tuner_id,packet);

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner " << tuner_id);
    }

    //Delete Memory
    delete packet;
    return true;
}

/* acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateRfFlowId(std::string tuner_type){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_type=" << tuner_type);

    std::string rf_flow_id;
    if( tuner_type == "RX_DIGITIZER")
        rf_flow_id = rx_rfinfo_pkt.rf_flow_id;
    else if( tuner_type == "TX")
        rf_flow_id = tx_rfinfo_pkt.rf_flow_id;
    else{
        LOG_WARN(USRP_UHD_i, std::string(__PRETTY_FUNCTION__) + ":: UNKNOWN TUNER TYPE: " + tuner_type);
        return;
    }

    for(size_t tuner_id = 0; tuner_id < frontend_tuner_status.size(); tuner_id++){
        if(frontend_tuner_status[tuner_id].tuner_type == tuner_type){

            interrupt(tuner_id);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
            exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

            frontend_tuner_status[tuner_id].rf_flow_id = rf_flow_id;
            usrp_tuners[tuner_id].update_sri = true;

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);
        }
    }
}

/* acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateGroupId(){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " device_group_id_global=" << device_group_id_global);
    for(size_t tuner_id = 0; tuner_id < frontend_tuner_status.size(); tuner_id++){

        interrupt(tuner_id);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

        frontend_tuner_status[tuner_id].group_id = device_group_id_global;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);
    }
}

/* This sets the number of entries in the frontend_tuner_status struct sequence property
 * as well as the tuner_allocation_ids vector. Only call this function during initialization
 */
void USRP_UHD_i::setNumChannels(size_t num){
    USRP_UHD_base::setNumChannels(num);
    usrp_tuners.clear();
    usrp_tuners.resize(num);
}

///////////////////////////////
//   CONFIGURE CALLBACKS     //
///////////////////////////////

void USRP_UHD_i::updateAvailableDevicesChanged(const bool* old_value, const bool* new_value){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "old_value=" << *old_value << "  new_value=" << *new_value);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "update_available_devices=" << update_available_devices);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    if (update_available_devices){
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: update_available_devices == true");
        updateAvailableDevices();
    }
    update_available_devices = false;
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}

void USRP_UHD_i::deviceIpAddressChanged(const std::string* old_value, const std::string* new_value) throw (CF::PropertySet::InvalidConfiguration) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "old_value=" << *old_value << "  new_value=" << *new_value);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device_ip_address=" << device_ip_address);

    if(started()){
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device has been started, must stop before initialization");
        stop();
    } else {
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device has not been started, continue with initialization");
    }

    { // scope for prop_lock
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
        exclusive_lock lock(prop_lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

        try{
            initUsrp();
        }catch(...){
            LOG_WARN(USRP_UHD_i,"CAUGHT EXCEPTION WHEN INITIALIZING USRP. WAITING 1 SECOND AND TRYING AGAIN");
            sleep(1);
            initUsrp();
        }
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
    } // end scope for prop_lock

    if(!started()){
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device is not started, must start device after initialization");
        start();
    } else {
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device was already started after initialization, not calling start again");
    }

}
void USRP_UHD_i::deviceGainChanged(const float* old_value, const float* new_value){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "old_value=" << *old_value << "  new_value=" << *new_value);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device_gain_global=" << device_gain_global);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    if (false){ // TODO - check validity of gain value, revert to old_value if new_value is invalid
        device_gain_global = *old_value;
        LOG_WARN(USRP_UHD_i,"Invalid gain value ("<< *new_value << "), ignoring.")
    } else {
        updateDeviceGain();
    }
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}

void USRP_UHD_i::deviceReferenceSourceChanged(const std::string* old_value, const std::string* new_value){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "old_value=" << *old_value << "  new_value=" << *new_value);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device_reference_source_global=" << device_reference_source_global);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateDeviceReferenceSource();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}

void USRP_UHD_i::deviceGroupIdChanged(const std::string* old_value, const std::string* new_value){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "old_value=" << *old_value << "  new_value=" << *new_value);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "device_group_id_global=" << device_group_id_global);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateGroupId();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}

/* acquire tuner's lock prior to calling this function */
std::string USRP_UHD_i::getStreamId(size_t tuner_id) {
    if (tuner_id >= usrp_tuners.size())
        return "ERR: INVALID TUNER ID";
    if (usrp_tuners[tuner_id].stream_id.empty()){
        std::ostringstream id;
        id<<"tuner_freq_"<<long(frontend_tuner_status[tuner_id].center_frequency)<<"_Hz_"<<frontend::uuidGenerator();
        usrp_tuners[tuner_id].stream_id = id.str();
        usrp_tuners[tuner_id].update_sri = true;
    }
    return usrp_tuners[tuner_id].stream_id;
}

/* acquire prop_lock prior to calling this function */
double USRP_UHD_i::optimizeRate(const double& req_rate, const size_t tuner_id){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " req_rate=" << req_rate);

    if(frontend::compareHz(req_rate,0) <= 0){
        return usrp_ranges[tuner_id].sample_rate.clip(device_channels[tuner_id].rate_min);
    }
    size_t dec = round(device_channels[tuner_id].clock_max/req_rate);
    double opt_rate = device_channels[tuner_id].clock_max / double(dec);
    double usrp_rate = usrp_ranges[tuner_id].sample_rate.clip(opt_rate);
    if(frontend::compareHz(usrp_rate,req_rate) >=0 ){
        return usrp_rate;
    }
    size_t min_dec = round(device_channels[tuner_id].clock_max/device_channels[tuner_id].rate_max);
    for(dec--; dec >= min_dec; dec--){
        opt_rate = device_channels[tuner_id].clock_max / double(dec);
        usrp_rate = usrp_ranges[tuner_id].sample_rate.clip(opt_rate);
        if(frontend::compareHz(usrp_rate,req_rate) >=0 ){
            return usrp_rate;
        }
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " could not optimize rate, returning req_rate (" << req_rate << ")");
    return req_rate;
}

/* acquire prop_lock prior to calling this function */
double USRP_UHD_i::optimizeBandwidth(const double& req_bw, const size_t tuner_id){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " req_bw=" << req_bw);

    if(frontend::compareHz(req_bw,0) <= 0){
        return usrp_ranges[tuner_id].bandwidth.clip(device_channels[tuner_id].bandwidth_min);
    }
    double usrp_bw = usrp_ranges[tuner_id].bandwidth.clip(req_bw);
    if(frontend::compareHz(usrp_bw,req_bw) >=0 ){
        return usrp_bw;
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " could not optimize rate, returning req_rate (" << req_bw << ")");
    return req_bw;
}

void USRP_UHD_i::updateSriTimes(BULKIO::StreamSRI *sri, double timeUp, double timeDown, frontend::timeTypes timeType) {

    std::stringstream DOIU;
    std::stringstream TUOI;
    std::stringstream DOID;
    std::stringstream TDOI;

    time_t time_up = (time_t) timeUp;
    time_t time_down = (time_t) timeDown;
    time_t now;
    struct tm curr_time;
    curr_time.tm_year = 0;
    struct tm gmt_up;
    struct tm gmt_down;

    gmt_up = *gmtime(&time_up);
    gmt_down = *gmtime(&time_down);
    if (timeType == frontend::JCY) {
        now = time(0);
        curr_time = *gmtime(&now);
    }

    DOIU.width(4);
    DOIU.fill('0');
    if (timeType == frontend::JCY)
        DOIU << (curr_time.tm_year + 1900);
    else
        DOIU << (gmt_up.tm_year + 1900);
    DOIU.width(2);
    DOIU.fill('0');
    DOIU << (gmt_up.tm_mon + 1);
    DOIU.width(2);
    DOIU.fill('0');
    DOIU << gmt_up.tm_mday;

    TUOI.width(2);
    TUOI.fill('0');
    TUOI << gmt_up.tm_hour;
    TUOI.width(2);
    TUOI.fill('0');
    TUOI << gmt_up.tm_min;
    TUOI.width(2);
    TUOI.fill('0');
    TUOI << gmt_up.tm_sec;

    DOID.width(4);
    DOID.fill('0');
    DOID << (gmt_down.tm_year + 1900);
    DOID.width(2);
    DOID.fill('0');
    DOID << (gmt_down.tm_mon + 1);
    DOID.width(2);
    DOID.fill('0');
    DOID << gmt_down.tm_mday;

    TDOI.width(2);
    TDOI.fill('0');
    TDOI << gmt_down.tm_hour;
    TDOI.width(2);
    TDOI.fill('0');
    TDOI << gmt_down.tm_min;
    TDOI.width(2);
    TDOI.fill('0');
    TDOI << gmt_down.tm_sec;


    addModifyKeyword<std::string > (sri, "DOIU", DOIU.str());
    addModifyKeyword<std::string > (sri, "TUOI", TUOI.str());
    addModifyKeyword<std::string > (sri, "DOID", DOID.str());
    addModifyKeyword<std::string > (sri, "TDOI", TDOI.str());
}

///////////////////////////////////
//   UPDATE USRP DEVICE INFO     //
///////////////////////////////////

/* call acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateAvailableDevices(){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);
    // EMPTY SEQ
    available_devices.clear();
    uhd::device_addr_t himt;
    uhd::device_addrs_t device_addrs = uhd::device::find(himt);
    if (device_addrs.empty())
        LOG_WARN(USRP_UHD_i, "WARNING: NO UHD (USRP) DEVICES FOUND!\n");
    for (size_t i = 0; i < device_addrs.size(); i++) {
        usrp_device_struct availDev;

        BOOST_FOREACH(std::string key, device_addrs[i].keys()) {
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "i=" << i << " key=" << key << " device_addrs[i].get(key)=" << device_addrs[i].get(key));
            if (key == "type")
                availDev.type = device_addrs[i].get(key);
            else if (key == "addr")
                availDev.ip_address = device_addrs[i].get(key);
            else if (key == "name")
                availDev.name = device_addrs[i].get(key);
            else if (key == "serial")
                availDev.serial = device_addrs[i].get(key);
        }
        available_devices.push_back(availDev);
    }
}

/* call acquire prop_lock prior to calling this function */
void USRP_UHD_i::initUsrp() throw (CF::PropertySet::InvalidConfiguration) {
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);

    try {
        usrp_device_addr = uhd::device_addr_t();
        usrp_device_addr["addr0"] = device_ip_address;
        usrp_device_ptr = uhd::usrp::multi_usrp::make(usrp_device_addr);

        // get_rx/tx_freq will throw an exception if set_rx/tx_freq has not already been called
        for (size_t chan = 0; chan < usrp_device_ptr->get_rx_num_channels(); chan++) {
            double setFreq = (usrp_device_ptr->get_rx_freq_range(chan).start() + usrp_device_ptr->get_rx_freq_range(chan).stop()) / 2;
            usrp_device_ptr->set_rx_freq(setFreq, chan);
        }
        for (size_t chan = 0; chan < usrp_device_ptr->get_tx_num_channels(); chan++) {
            double setFreq = (usrp_device_ptr->get_tx_freq_range(chan).start() + usrp_device_ptr->get_tx_freq_range(chan).stop()) / 2;
            usrp_device_ptr->set_tx_freq(setFreq, chan);
        }

        struct timeval tmp_time;
        struct timezone tmp_tz;
        gettimeofday(&tmp_time, &tmp_tz);
        time_t wsec = tmp_time.tv_sec;
        double fsec = tmp_time.tv_usec / 1e6;
        usrp_device_ptr->set_time_now(uhd::time_spec_t(wsec,fsec));

        // This will update property structures that describe the USRP device (motherboard + daughtercards)
        updateDeviceInfo();

        // Initialize tasking and status vectors
        setNumChannels(device_channels.size());

        //Initialize Data Members
        long source_prop = 0;
        if(device_reference_source_global != "INTERNAL"){
            source_prop = 1;
        }
        char tmp[128];
        for (size_t tuner_id = 0; tuner_id < device_channels.size(); tuner_id++) {
            if (usrp_tuners[tuner_id].lock == NULL)
                usrp_tuners[tuner_id].lock = new boost::mutex;

            frontend_tuner_status[tuner_id].allocation_id_csv = "";
            frontend_tuner_status[tuner_id].tuner_type = device_channels[tuner_id].tuner_type;
            frontend_tuner_status[tuner_id].center_frequency = device_channels[tuner_id].freq_current;
            frontend_tuner_status[tuner_id].sample_rate = device_channels[tuner_id].rate_current;
            frontend_tuner_status[tuner_id].bandwidth = device_channels[tuner_id].bandwidth_current;
            if( device_channels[tuner_id].tuner_type == "RX_DIGITIZER")
                frontend_tuner_status[tuner_id].rf_flow_id = rx_rfinfo_pkt.rf_flow_id;
            else if( device_channels[tuner_id].tuner_type == "TX")
                frontend_tuner_status[tuner_id].rf_flow_id = tx_rfinfo_pkt.rf_flow_id;
            frontend_tuner_status[tuner_id].reference_source = source_prop;
            frontend_tuner_status[tuner_id].gain = device_channels[tuner_id].gain_current;
            frontend_tuner_status[tuner_id].group_id = device_group_id_global;

            frontend_tuner_status[tuner_id].tuner_number = tuner_id;
            frontend_tuner_status[tuner_id].enabled = false;
            frontend_tuner_status[tuner_id].complex = true;
            frontend_tuner_status[tuner_id].valid = true;
            frontend_tuner_status[tuner_id].sample_rate_tolerance = 0.0;
            frontend_tuner_status[tuner_id].bandwidth_tolerance = 0.0;

            if( frontend::compareHz(device_channels[tuner_id].freq_min,device_channels[tuner_id].freq_max) < 0 )
                sprintf(tmp,"%.2f-%.2f",device_channels[tuner_id].freq_min,device_channels[tuner_id].freq_max);
            else
                sprintf(tmp,"%.2f",device_channels[tuner_id].freq_min);
            frontend_tuner_status[tuner_id].available_frequency = std::string(tmp);
            if( device_channels[tuner_id].gain_min < device_channels[tuner_id].gain_max )
                sprintf(tmp,"%.2f-%.2f",device_channels[tuner_id].gain_min,device_channels[tuner_id].gain_max);
            else
                sprintf(tmp,"%.2f",device_channels[tuner_id].gain_min);
            frontend_tuner_status[tuner_id].available_gain = std::string(tmp);
            if( frontend::compareHz(device_channels[tuner_id].rate_min,device_channels[tuner_id].rate_max) < 0 )
                sprintf(tmp,"%.2f-%.2f",device_channels[tuner_id].rate_min,device_channels[tuner_id].rate_max);
            else
                sprintf(tmp,"%.2f",device_channels[tuner_id].rate_min);
            frontend_tuner_status[tuner_id].available_sample_rate = std::string(tmp);
            //sprintf(tmp,"%.2f",40e6); // fixed bandwidth of 40 MHz
            //frontend_tuner_status[tuner_id].available_bandwidth = std::string(tmp);
            if( frontend::compareHz(device_channels[tuner_id].bandwidth_min,device_channels[tuner_id].bandwidth_max) < 0 )
                sprintf(tmp,"%.2f-%.2f",device_channels[tuner_id].bandwidth_min,device_channels[tuner_id].bandwidth_max);
            else
                sprintf(tmp,"%.2f",device_channels[tuner_id].bandwidth_min);
            frontend_tuner_status[tuner_id].available_bandwidth = std::string(tmp);
        }

        // update device channels with global settings
        //updateGroupId(); // already completed above, nothing extra needs to be done
        updateDeviceGain(); // sets device with global value, so need to call this one
        updateDeviceReferenceSource(); // sets device with global value, so need to call this one

    } catch (...) {
        LOG_ERROR(USRP_UHD_i,"USRP COULD NOT BE INITIALIZED!");
        throw CF::PropertySet::InvalidConfiguration();
    }
}

/* acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateDeviceInfo() {
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__);
    if (usrp_device_ptr.get() == NULL)
        return;

    device_motherboards.clear();
    for (size_t mthr = 0; mthr < usrp_device_ptr->get_num_mboards(); mthr++) {
        usrp_motherboard_struct availMotherboard;
        availMotherboard.mb_name = usrp_device_ptr->get_mboard_name(mthr);
        availMotherboard.mb_ip = "[NEED TO IMPLEMENT]";
        device_motherboards.push_back(availMotherboard);
    }

    device_channels.clear();
    size_t num_rx_channels = usrp_device_ptr->get_rx_num_channels();
    size_t num_tx_channels = usrp_device_ptr->get_tx_num_channels();
    usrp_ranges.resize(num_rx_channels+num_tx_channels);
    for (size_t chan = 0; chan < num_rx_channels; chan++) {
        usrp_channel_struct availChan;
        availChan.chan_num = chan;
        availChan.ch_name = usrp_device_ptr->get_rx_subdev_name(chan);
        availChan.tuner_type = "RX_DIGITIZER";
        if(availChan.ch_name.find("unknown") != std::string::npos)
            availChan.tuner_type = "UNKNOWN";
        availChan.antenna = usrp_device_ptr->get_rx_antenna(chan);

        std::vector<double> rates = usrp_device_ptr->get_rx_dboard_iface(chan)->get_clock_rates(uhd::usrp::dboard_iface::UNIT_RX);
        availChan.clock_min = rates.back();
        availChan.clock_max = rates.front();

        availChan.freq_current = usrp_device_ptr->get_rx_freq(chan);
        usrp_ranges[chan].frequency = usrp_device_ptr->get_rx_freq_range(chan);
        availChan.freq_min = usrp_ranges[chan].frequency.start();
        availChan.freq_max = usrp_ranges[chan].frequency.stop();

        availChan.bandwidth_current = usrp_device_ptr->get_rx_bandwidth(chan);
        usrp_ranges[chan].bandwidth = usrp_device_ptr->get_rx_bandwidth_range(chan);
        availChan.bandwidth_min = usrp_ranges[chan].bandwidth.start();
        availChan.bandwidth_max = usrp_ranges[chan].bandwidth.stop();

        availChan.rate_current = usrp_device_ptr->get_rx_rate(chan);
        usrp_ranges[chan].sample_rate = usrp_device_ptr->get_rx_rates(chan);
        availChan.rate_min = usrp_ranges[chan].sample_rate.start();
        availChan.rate_max = usrp_ranges[chan].sample_rate.stop();

        availChan.gain_current = usrp_device_ptr->get_rx_gain(chan);
        usrp_ranges[chan].gain = usrp_device_ptr->get_rx_gain_range(chan);
        availChan.gain_min = usrp_ranges[chan].gain.start();
        availChan.gain_max = usrp_ranges[chan].gain.stop();

        device_channels.push_back(availChan);
    }
    for (size_t chan = 0; chan < num_tx_channels; chan++) {
        usrp_channel_struct availChan;
        availChan.chan_num = chan;
        availChan.ch_name = usrp_device_ptr->get_tx_subdev_name(chan);
        availChan.tuner_type = "TX";
        if(availChan.ch_name.find("unknown") != std::string::npos)
            availChan.tuner_type = "UNKNOWN";
        availChan.antenna = usrp_device_ptr->get_tx_antenna(chan);

        std::vector<double> rates = usrp_device_ptr->get_tx_dboard_iface(chan)->get_clock_rates(uhd::usrp::dboard_iface::UNIT_TX);
        availChan.clock_min = rates.back();
        availChan.clock_max = rates.front();

        availChan.freq_current = usrp_device_ptr->get_tx_freq(chan);
        usrp_ranges[num_rx_channels+chan].frequency = usrp_device_ptr->get_tx_freq_range(chan);
        availChan.freq_min = usrp_ranges[num_rx_channels+chan].frequency.start();
        availChan.freq_max = usrp_ranges[num_rx_channels+chan].frequency.stop();

        availChan.bandwidth_current = usrp_device_ptr->get_tx_bandwidth(chan);
        usrp_ranges[num_rx_channels+chan].bandwidth = usrp_device_ptr->get_tx_bandwidth_range(chan);
        availChan.bandwidth_min = usrp_ranges[num_rx_channels+chan].bandwidth.start();
        availChan.bandwidth_max = usrp_ranges[num_rx_channels+chan].bandwidth.stop();

        availChan.rate_current = usrp_device_ptr->get_tx_rate(chan);
        usrp_ranges[num_rx_channels+chan].sample_rate = usrp_device_ptr->get_tx_rates(chan);
        availChan.rate_min = usrp_ranges[num_rx_channels+chan].sample_rate.start();
        availChan.rate_max = usrp_ranges[num_rx_channels+chan].sample_rate.stop();

        availChan.gain_current = usrp_device_ptr->get_tx_gain(chan);
        usrp_ranges[num_rx_channels+chan].gain = usrp_device_ptr->get_tx_gain_range(chan);
        availChan.gain_min = usrp_ranges[num_rx_channels+chan].gain.start();
        availChan.gain_max = usrp_ranges[num_rx_channels+chan].gain.stop();

        device_channels.push_back(availChan);
    }
}

/* acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateDeviceGain() {
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " device_gain_global=" << device_gain_global);

    if (usrp_device_ptr.get() == NULL)
        return;

    for(size_t tuner_id = 0; tuner_id < frontend_tuner_status.size(); tuner_id++){
        if(frontend_tuner_status[tuner_id].tuner_type == "RX_DIGITIZER"){
            size_t usrp_channel_number = device_channels[tuner_id].chan_num;

            interrupt(tuner_id);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " waiting for tuner lock for tuner_id " << tuner_id);
            exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " acquired tuner lock for tuner_id " << tuner_id);

            usrp_device_ptr->set_rx_gain(device_gain_global,usrp_channel_number);
            frontend_tuner_status[tuner_id].gain = usrp_device_ptr->get_rx_gain(usrp_channel_number);

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " releasing tuner lock for tuner_id " << tuner_id);
        }
    }
}

/* acquire prop_lock prior to calling this function */
void USRP_UHD_i::updateDeviceReferenceSource(){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " device_reference_source_global=" << device_reference_source_global);

    if (usrp_device_ptr.get() == NULL)
        return;

    long source_prop = 0;
    if(device_reference_source_global != "INTERNAL"){
        source_prop = 1;
    }
    for(size_t tuner_id = 0; tuner_id < frontend_tuner_status.size(); tuner_id++){

        interrupt(tuner_id);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << tuner_id);
        exclusive_lock tuner_lock(*usrp_tuners[tuner_id].lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << tuner_id);

        frontend_tuner_status[tuner_id].reference_source = source_prop;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id " << tuner_id);
    }

    // TODO - disable enabled tuners first? stop/restart device?

    if (device_reference_source_global == "MIMO") {
        usrp_device_ptr->set_clock_source("MIMO",0);
        usrp_device_ptr->set_time_source("MIMO",0);
    } else if (device_reference_source_global == "EXTERNAL") {
        usrp_device_ptr->set_clock_source("external",0);
        usrp_device_ptr->set_time_source("external",0);
    } else if (device_reference_source_global == "INTERNAL") {
        usrp_device_ptr->set_clock_source("internal",0);
        usrp_device_ptr->set_time_source("external",0);
    }
}

/* acquire tuner_lock prior to calling this function *
 * this function will block up to "timeout" seconds
 */
bool USRP_UHD_i::usrpReceive(size_t tuner_id, double timeout){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id);

    // calc num samps to rx based on timeout, sr, and buffer size
    size_t samps_to_rx = std::min(
            size_t(timeout*frontend_tuner_status[tuner_id].sample_rate),
            size_t((usrp_tuners[tuner_id].buffer_capacity-usrp_tuners[tuner_id].buffer_size) / 2));

    uhd::rx_metadata_t _metadata;
    //size_t max_pkt_samps = usrp_device_ptr->get_device()->get_max_recv_samps_per_packet();
    //LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " BEFORE RECV -- max_pkt_samps=" << max_pkt_samps);
    //LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " BEFORE RECV -- samps_to_rx=" << samps_to_rx);
    //time_t t1 = time(0);
    //std::clock_t c1 = clock();
    size_t num_samps = usrp_device_ptr->get_device()->recv(
            &usrp_tuners[tuner_id].output_buffer.at(usrp_tuners[tuner_id].buffer_size), // address of buffer to start filling data
            //(usrp_tuners[tuner_id].buffer_capacity-usrp_tuners[tuner_id].buffer_size) / 2, // number of samples buffer can fit
            samps_to_rx,
            _metadata,
            uhd::io_type_t::COMPLEX_INT16,
            uhd::device::RECV_MODE_FULL_BUFF);
    //std::clock_t c2 = clock();
    //time_t t2 = time(0);
    //LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " AFTER RECV -- seconds: " << double(t2-t1) << " clocks: " << double(c2-c1));
    //LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " AFTER RECV -- num_samps: " << num_samps);
    usrp_tuners[tuner_id].buffer_size += (num_samps*2);

//        /* TODO -- replace line above with new method below, allowing 8- or 16-bit CI data from USRP*/
//        /* TODO -- also allows specification of which channel to receive from, rather than defaulting to 0*/
//        /* TODO -- similar code to replace TX code*/
//
//        // do this during initUsrp
//        //create a receive streamer
//        /*!
//         * The CPU format is a string that describes the format of host memory.
//         * Conversions for the following CPU formats have been implemented:
//         *  - fc64 - complex<double>
//         *  - fc32 - complex<float>
//         *  - sc16 - complex<int16_t>
//         *  - sc8 - complex<int8_t>
//         */
//        std::string cpu_format = "sc16";
//
//        /*!
//         * The OTW format is a string that describes the format over-the-wire.
//         * The following over-the-wire formats have been implemented:
//         *  - sc16 - Q16 I16
//         *  - sc8 - Q8_1 I8_1 Q8_0 I8_0
//         */
//        std::string wire_format = "sc8";
//
//        size_t chan = device_channels[tuner_id].chan_num;
//        uhd::stream_args_t stream_args(cpu_format,wire_format);
//        stream_args.channels.push_back(chan);
//        uhd::rx_streamer::sptr rx_stream = usrp_device_ptr->get_rx_stream(stream_args);
//
//        // this is for the deviceEnable function (similar needed in deviceDisable function)
//        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
//        stream_cmd.num_samps = 0;
//        stream_cmd.stream_now = true;
//        stream_cmd.time_spec = uhd::time_spec_t();
//        usrp_device_ptr->issue_stream_cmd(stream_cmd, chan);
//
//        size_t num_samps = rx_stream->recv(&usrp_tuners[tuner_id].output_buffer.front(), usrp_tuners[tuner_id].output_buffer.size() / 2, _metadata, 3.0);
//        /* END TODO */

    //handle possible errors conditions
    switch (_metadata.error_code) {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            break;
        case uhd::rx_metadata_t::ERROR_CODE_TIMEOUT:
            LOG_WARN(USRP_UHD_i,"WARNING: TIMEOUT OCCURED ON USRP RECEIVE! (received num_samps=" << num_samps << ")");
            return false;
        case uhd::rx_metadata_t::ERROR_CODE_OVERFLOW:
            LOG_WARN(USRP_UHD_i,"WARNING: USRP OVERFLOW DETECTED!");
            // may have received data, but 0 is returned by usrp recv function so we don't know how many samples, must throw away
            return true; // this will just cause us to return NORMAL so there's no wait before next iteration
        default:
            LOG_WARN(USRP_UHD_i,"WARNING: UHD source block got error code 0x" << _metadata.error_code);
            return false;
    }

    if(num_samps == 0)
        return false;

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "  received data.  num_samps=" << num_samps
                                         << "  buffer_size=" << usrp_tuners[tuner_id].buffer_size
                                         << "  buffer_capacity=" << usrp_tuners[tuner_id].buffer_capacity );

    // if first samples in buffer, update timestamps
    if(num_samps*2 == usrp_tuners[tuner_id].buffer_size){
        usrp_tuners[tuner_id].output_buffer_time = bulkio::time::utils::now();
        usrp_tuners[tuner_id].output_buffer_time.twsec = _metadata.time_spec.get_real_secs();
        usrp_tuners[tuner_id].output_buffer_time.tfsec = _metadata.time_spec.get_frac_secs();
        if (usrp_tuners[tuner_id].time_up.twsec <= 0)
            usrp_tuners[tuner_id].time_up = usrp_tuners[tuner_id].output_buffer_time;
        usrp_tuners[tuner_id].time_down = usrp_tuners[tuner_id].output_buffer_time;
    }

    return true;
}


/* acquire tuner_lock prior to calling this function *
 */
template <class PACKET_TYPE> bool USRP_UHD_i::usrpTransmit(size_t tuner_id, PACKET_TYPE *packet){//, double timeout){

    if(usrp_tuners[tuner_id].update_sri){
        tx_rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        tx_rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        tx_rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " pushing tx_rfinfo_pkt w/ freq=" << tx_rfinfo_pkt.rf_center_freq << " and bw=" << tx_rfinfo_pkt.rf_bandwidth);
        RFInfoTX_out->rfinfo_pkt(tx_rfinfo_pkt);
        usrp_tuners[tuner_id].update_sri = false;
    }

    //Sets basic data type. IE- float for float port, short for short port
    typedef typeof (packet->dataBuffer[0]) PACKET_ELEMENT_TYPE;

    uhd::tx_metadata_t _metadata;
    _metadata.start_of_burst = false;
    _metadata.end_of_burst = false;

    // Send in size/2 because it is complex
    if (sizeof (PACKET_ELEMENT_TYPE) == 2){
        if( usrp_device_ptr->get_device()->send(&packet->dataBuffer.front(), packet->dataBuffer.size() / 2, _metadata, uhd::io_type_t::COMPLEX_INT16, uhd::device::SEND_MODE_FULL_BUFF) != packet->dataBuffer.size() / 2){
            LOG_WARN(USRP_UHD_i, "WARNING: THE USRP WAS UNABLE TO TRANSMIT " << size_t(packet->dataBuffer.size()) / 2 << " NUMBER OF COMPLEX SHORT SAMPLES!");
            return false;
        }
    }
    else if (sizeof (PACKET_ELEMENT_TYPE) == 4){
        if( usrp_device_ptr->get_device()->send(&packet->dataBuffer.front(), packet->dataBuffer.size() / 2, _metadata, uhd::io_type_t::COMPLEX_FLOAT32, uhd::device::SEND_MODE_FULL_BUFF) != packet->dataBuffer.size() / 2){
            LOG_WARN(USRP_UHD_i, "WARNING: THE USRP WAS UNABLE TO TRANSMIT " << size_t(packet->dataBuffer.size()) / 2 << " NUMBER OF COMPLEX FLOAT SAMPLES!");
            return false;
        }
    }
    return true;
}

/* acquire tuner's lock prior to calling this function */
bool USRP_UHD_i::usrpEnable(size_t tuner_id, size_t chan){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " chan=" << chan);
    //if (tuner_id >= sim_tuners.size())
    //    throw FRONTEND::BadParameterException("usrpEnable: INVALID TUNER ID");

    bool prev_enabled = frontend_tuner_status[tuner_id].enabled;
    frontend_tuner_status[tuner_id].enabled = true;

    if(frontend_tuner_status[tuner_id].tuner_type == "TX"){

        tx_rfinfo_pkt.rf_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        tx_rfinfo_pkt.if_center_freq = frontend_tuner_status[tuner_id].center_frequency;
        tx_rfinfo_pkt.rf_bandwidth = frontend_tuner_status[tuner_id].bandwidth;

        if(!prev_enabled){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " pushing tx_rfinfo_pkt w/ freq=" << tx_rfinfo_pkt.rf_center_freq << " and bw=" << tx_rfinfo_pkt.rf_bandwidth);
            RFInfoTX_out->rfinfo_pkt(tx_rfinfo_pkt);
            usrp_tuners[tuner_id].update_sri = false;
        }

    } else {

        // get stream id (creates one if not already created for this tuner)
        std::string stream_id = getStreamId(tuner_id);

        if(!prev_enabled){
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " pushing SRI for stream_id=" << stream_id);
            BULKIO::StreamSRI sri = create(stream_id, frontend_tuner_status[tuner_id]);
            sri.mode = 1; // complex
            //printSRI(&sri); // DEBUG
            dataShort_out->pushSRI(sri);
            usrp_tuners[tuner_id].update_sri = false;
        }

        usrp_device_ptr->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS, chan);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " started stream_id=" << stream_id);
    }
    return true;
}

/* acquire tuner's lock prior to calling this function */
bool USRP_UHD_i::usrpDisable(size_t tuner_id, size_t chan){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " tuner_id=" << tuner_id << " chan=" << chan);
    //if (tuner_id >= sim_tuners.size())
    //    throw FRONTEND::BadParameterException("usrpDisable: INVALID TUNER ID");

    bool prev_enabled = frontend_tuner_status[tuner_id].enabled;
    frontend_tuner_status[tuner_id].enabled = false;

    if(frontend_tuner_status[tuner_id].tuner_type != "TX"){
        usrp_device_ptr->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,chan);

        if(prev_enabled){
            // get stream id (creates one if not already created for this tuner)
            std::string stream_id = getStreamId(tuner_id);
            BULKIO::StreamSRI sri = create(stream_id, frontend_tuner_status[tuner_id]);
            sri.mode = 1; // complex
            // TODO - does updateSriTimes need to be called each time SRI is pushed?
            updateSriTimes(&sri, usrp_tuners[tuner_id].time_up.twsec, usrp_tuners[tuner_id].time_down.twsec, frontend::J1970);
            dataShort_out->pushSRI(sri);
            usrp_tuners[tuner_id].update_sri = false;

            usrp_tuners[tuner_id].output_buffer.resize(usrp_tuners[tuner_id].buffer_size);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "  pushing EOS with remaining samples."
                                                 << "  buffer_size=" << usrp_tuners[tuner_id].buffer_size
                                                 << "  buffer_capacity=" << usrp_tuners[tuner_id].buffer_capacity
                                                 << "  output_buffer.size()=" << usrp_tuners[tuner_id].output_buffer.size() );
            dataShort_out->pushPacket(usrp_tuners[tuner_id].output_buffer, usrp_tuners[tuner_id].output_buffer_time, true, stream_id);
            usrp_tuners[tuner_id].buffer_size = 0;
            usrp_tuners[tuner_id].output_buffer.resize(usrp_tuners[tuner_id].buffer_capacity);
        }

        bulkio::sri::zeroTime(usrp_tuners[tuner_id].output_buffer_time);
        bulkio::sri::zeroTime(usrp_tuners[tuner_id].time_up);
        bulkio::sri::zeroTime(usrp_tuners[tuner_id].time_down);
    }
    return true;
}

/*************************************************************
Functions servicing the RFInfo port(s)
- port_name is the port over which the call was received
*************************************************************/
std::string USRP_UHD_i::get_rf_flow_id(const std::string& port_name){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " port_name=" << port_name);

    if( port_name == "RFInfo_in"){
        return rx_rfinfo_pkt.rf_flow_id;
    } else if( port_name == "RFInfoTX_out"){
        return tx_rfinfo_pkt.rf_flow_id;
    } else {
        LOG_WARN(USRP_UHD_i, std::string(__PRETTY_FUNCTION__) + ":: UNKNOWN PORT NAME: " + port_name);
        return "";
    }
}
void USRP_UHD_i::set_rf_flow_id(const std::string& port_name, const std::string& id){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " port_name=" << port_name << " id=" << id);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    if( port_name == "RFInfo_in"){
        rx_rfinfo_pkt.rf_flow_id = id;
        updateRfFlowId("RX_DIGITIZER");
    } else if( port_name == "RFInfoTX_out"){
        tx_rfinfo_pkt.rf_flow_id = id;
        updateRfFlowId("TX");
    } else {
        LOG_WARN(USRP_UHD_i, std::string(__PRETTY_FUNCTION__) + ":: UNKNOWN PORT NAME: " + port_name);
    }
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}
frontend::RFInfoPkt USRP_UHD_i::get_rfinfo_pkt(const std::string& port_name){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " port_name=" << port_name);

    frontend::RFInfoPkt tmp;
    frontend::RFInfoPkt* pkt;
    if( port_name == "RFInfo_in"){
        pkt = &rx_rfinfo_pkt;
    } else if( port_name == "RFInfoTX_out"){
        pkt = &tx_rfinfo_pkt;
    } else {
        LOG_WARN(USRP_UHD_i, std::string(__PRETTY_FUNCTION__) + ":: UNKNOWN PORT NAME: " + port_name);
        return tmp;
    }
    tmp.rf_flow_id = pkt->rf_flow_id;
    tmp.rf_center_freq = pkt->rf_center_freq;
    tmp.rf_bandwidth = pkt->rf_bandwidth;
    tmp.if_center_freq = pkt->if_center_freq;
    tmp.spectrum_inverted = pkt->spectrum_inverted;
    tmp.sensor.collector = pkt->sensor.collector;
    tmp.sensor.mission = pkt->sensor.mission;
    tmp.sensor.rx = pkt->sensor.rx;
    tmp.sensor.antenna.description = pkt->sensor.antenna.description;
    tmp.sensor.antenna.name = pkt->sensor.antenna.name;
    tmp.sensor.antenna.size = pkt->sensor.antenna.size;
    tmp.sensor.antenna.type = pkt->sensor.antenna.type;
    tmp.sensor.feed.name = pkt->sensor.feed.name;
    tmp.sensor.feed.polarization = pkt->sensor.feed.polarization;
    tmp.sensor.feed.freq_range.max_val = pkt->sensor.feed.freq_range.max_val;
    tmp.sensor.feed.freq_range.min_val = pkt->sensor.feed.freq_range.min_val;
    tmp.sensor.feed.freq_range.values.resize(pkt->sensor.feed.freq_range.values.size());
    for (unsigned int i=0; i<pkt->sensor.feed.freq_range.values.size(); i++) {
        tmp.sensor.feed.freq_range.values[i] = pkt->sensor.feed.freq_range.values[i];
    }
    return tmp;
}
void USRP_UHD_i::set_rfinfo_pkt(const std::string& port_name, const frontend::RFInfoPkt &pkt){
    LOG_TRACE(USRP_UHD_i,__PRETTY_FUNCTION__ << " port_name=" << port_name << " pkt.rf_flow_id=" << pkt.rf_flow_id);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    if( port_name == "RFInfo_in"){
        rx_rfinfo_pkt.rf_flow_id = pkt.rf_flow_id;
        rx_rfinfo_pkt.rf_center_freq = pkt.rf_center_freq;
        rx_rfinfo_pkt.rf_bandwidth = pkt.rf_bandwidth;
        rx_rfinfo_pkt.if_center_freq = pkt.if_center_freq;
        rx_rfinfo_pkt.spectrum_inverted = pkt.spectrum_inverted;
        rx_rfinfo_pkt.sensor.collector = pkt.sensor.collector;
        rx_rfinfo_pkt.sensor.mission = pkt.sensor.mission;
        rx_rfinfo_pkt.sensor.rx = pkt.sensor.rx;
        rx_rfinfo_pkt.sensor.antenna.description = pkt.sensor.antenna.description;
        rx_rfinfo_pkt.sensor.antenna.name = pkt.sensor.antenna.name;
        rx_rfinfo_pkt.sensor.antenna.size = pkt.sensor.antenna.size;
        rx_rfinfo_pkt.sensor.antenna.type = pkt.sensor.antenna.type;
        rx_rfinfo_pkt.sensor.feed.name = pkt.sensor.feed.name;
        rx_rfinfo_pkt.sensor.feed.polarization = pkt.sensor.feed.polarization;
        rx_rfinfo_pkt.sensor.feed.freq_range.max_val = pkt.sensor.feed.freq_range.max_val;
        rx_rfinfo_pkt.sensor.feed.freq_range.min_val = pkt.sensor.feed.freq_range.min_val;
        rx_rfinfo_pkt.sensor.feed.freq_range.values.resize(pkt.sensor.feed.freq_range.values.size());
        for (unsigned int i=0; i<pkt.sensor.feed.freq_range.values.size(); i++) {
            rx_rfinfo_pkt.sensor.feed.freq_range.values[i] = pkt.sensor.feed.freq_range.values[i];
        }
        updateRfFlowId("RX_DIGITIZER");
    } else if( port_name == "RFInfoTX_out"){
        tx_rfinfo_pkt.rf_flow_id = pkt.rf_flow_id;
        tx_rfinfo_pkt.rf_center_freq = pkt.rf_center_freq;
        tx_rfinfo_pkt.rf_bandwidth = pkt.rf_bandwidth;
        tx_rfinfo_pkt.if_center_freq = pkt.if_center_freq;
        tx_rfinfo_pkt.spectrum_inverted = pkt.spectrum_inverted;
        tx_rfinfo_pkt.sensor.collector = pkt.sensor.collector;
        tx_rfinfo_pkt.sensor.mission = pkt.sensor.mission;
        tx_rfinfo_pkt.sensor.rx = pkt.sensor.rx;
        tx_rfinfo_pkt.sensor.antenna.description = pkt.sensor.antenna.description;
        tx_rfinfo_pkt.sensor.antenna.name = pkt.sensor.antenna.name;
        tx_rfinfo_pkt.sensor.antenna.size = pkt.sensor.antenna.size;
        tx_rfinfo_pkt.sensor.antenna.type = pkt.sensor.antenna.type;
        tx_rfinfo_pkt.sensor.feed.name = pkt.sensor.feed.name;
        tx_rfinfo_pkt.sensor.feed.polarization = pkt.sensor.feed.polarization;
        tx_rfinfo_pkt.sensor.feed.freq_range.max_val = pkt.sensor.feed.freq_range.max_val;
        tx_rfinfo_pkt.sensor.feed.freq_range.min_val = pkt.sensor.feed.freq_range.min_val;
        tx_rfinfo_pkt.sensor.feed.freq_range.values.resize(pkt.sensor.feed.freq_range.values.size());
        for (unsigned int i=0; i<pkt.sensor.feed.freq_range.values.size(); i++) {
            tx_rfinfo_pkt.sensor.feed.freq_range.values[i] = pkt.sensor.feed.freq_range.values[i];
        }
        updateRfFlowId("TX");
    } else {
        LOG_WARN(USRP_UHD_i, std::string(__PRETTY_FUNCTION__) + ":: UNKNOWN PORT NAME: " + port_name);
    }
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}
/*************************************************************
Functions servicing the tuner control port
*************************************************************/
std::string USRP_UHD_i::getTunerType(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " allocation_id=" << allocation_id);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].tuner_type;
}
bool USRP_UHD_i::getTunerDeviceControl(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " allocation_id=" << allocation_id);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    if (this->getControlAllocationId(idx) == allocation_id)
        return true;
    return false;
}
std::string USRP_UHD_i::getTunerGroupId(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " allocation_id=" << allocation_id);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].group_id;
}
std::string USRP_UHD_i::getTunerRfFlowId(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " allocation_id=" << allocation_id);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].rf_flow_id;
}
void USRP_UHD_i::setTunerCenterFrequency(const std::string& allocation_id, double freq) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " allocation_id=" << allocation_id << " freq=" << freq);

    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx)){
        LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: ID (" << allocation_id << ") DOES NOT HAVE AUTHORIZATION TO MODIFY TUNER!" );
        throw FRONTEND::FrontendException((std::string(__PRETTY_FUNCTION__)+" - ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    }
    try {

        size_t usrp_channel_number=-1;
        try {
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
            exclusive_lock lock(prop_lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

            if (freq < device_channels[idx].freq_min || freq > device_channels[idx].freq_max){
                std::ostringstream msg;
                msg << __PRETTY_FUNCTION__ << ":: INVALID CENTER FREQUENCY (" << freq <<")";
                LOG_WARN(USRP_UHD_i,msg.str() );
                LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
                throw FRONTEND::BadParameterException(msg.str().c_str());
            }
            usrp_channel_number = device_channels[idx].chan_num;
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
        } catch (FRONTEND::BadParameterException) {
            throw;
        } catch (...) {
            LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
            throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
        }

        if (frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // If the freq has changed (change in stream) or the tuner is disabled, then set it as disabled
            bool is_tuner_enabled = frontend_tuner_status[idx].enabled;
            if (usrp_device_ptr->get_rx_freq(usrp_channel_number) != freq)
                usrpDisable(idx, usrp_channel_number);

            // set hw with new value
            usrp_device_ptr->set_rx_freq(freq, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].center_frequency = usrp_device_ptr->get_rx_freq(usrp_channel_number);
            usrp_tuners[idx].update_sri = true;

            // re-enable
            if (is_tuner_enabled)
                usrpEnable(idx, usrp_channel_number);

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else if (frontend_tuner_status[idx].tuner_type == "TX") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // set hw with new value
            usrp_device_ptr->set_tx_freq(freq, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].center_frequency = usrp_device_ptr->get_tx_freq(usrp_channel_number);

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else {
            LOG_ERROR(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__)+" : INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!").c_str());
        }
    } catch (std::exception& e) {
        LOG_WARN(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " EXCEPTION:: " + std::string(e.what()) );
        throw FRONTEND::FrontendException(("WARNING: " + std::string(e.what())).c_str());
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateDeviceInfo();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}
double USRP_UHD_i::getTunerCenterFrequency(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].center_frequency;
}
void USRP_UHD_i::setTunerBandwidth(const std::string& allocation_id, double bw) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);

    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx)){
        LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: ID (" << allocation_id << ") DOES NOT HAVE AUTHORIZATION TO MODIFY TUNER!" );
        throw FRONTEND::FrontendException((std::string(__PRETTY_FUNCTION__)+" - ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    }
    try {
        size_t usrp_channel_number=-1;
        double opt_bw = bw;
        try {
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
            exclusive_lock lock(prop_lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

            if (frontend::compareHz(bw,0.0) < 0 ||
                    frontend::compareHz(bw,device_channels[idx].bandwidth_max) > 0 ){
                std::ostringstream msg;
                msg << __PRETTY_FUNCTION__ << ":: INVALID BANDWIDTH (" << bw <<")";
                LOG_WARN(USRP_UHD_i,msg.str() );
                LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
                throw FRONTEND::BadParameterException(msg.str().c_str());
            }
            opt_bw = optimizeBandwidth(bw, idx);
            usrp_channel_number = device_channels[idx].chan_num;
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
        } catch (FRONTEND::BadParameterException) {
            throw;
        } catch (...) {
            LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
            throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
        }

        if (frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // set hw with new value
            usrp_device_ptr->set_rx_bandwidth(opt_bw, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].bandwidth = usrp_device_ptr->get_rx_bandwidth(usrp_channel_number);
            usrp_tuners[idx].update_sri = true;

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else if (frontend_tuner_status[idx].tuner_type == "TX") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // set hw with new value
            usrp_device_ptr->set_tx_bandwidth(opt_bw, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].bandwidth = usrp_device_ptr->get_tx_bandwidth(usrp_channel_number);

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else {
            LOG_ERROR(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__)+" : INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!").c_str());
        }

    } catch (std::exception& e) {
        LOG_WARN(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " EXCEPTION:: " + std::string(e.what()) );
        throw FRONTEND::FrontendException(("WARNING: " + std::string(e.what())).c_str());
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateDeviceInfo();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}
double USRP_UHD_i::getTunerBandwidth(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].bandwidth;
}
void USRP_UHD_i::setTunerAgcEnable(const std::string& allocation_id, bool enable){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    throw FRONTEND::NotSupportedException("setTunerAgcEnable not supported");
}
bool USRP_UHD_i::getTunerAgcEnable(const std::string& allocation_id){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    throw FRONTEND::NotSupportedException("getTunerAgcEnable not supported");
}
void USRP_UHD_i::setTunerGain(const std::string& allocation_id, float gain){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " - Gain setting is global for all tuners. Use device property interface instead.")
    throw FRONTEND::NotSupportedException("setTunerGain not supported for individual channels. Use device property interface instead.");
}
float USRP_UHD_i::getTunerGain(const std::string& allocation_id){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].gain;
}
void USRP_UHD_i::setTunerReferenceSource(const std::string& allocation_id, long source){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " - Reference source setting is global for all tuners. Use device property interface instead.")
    throw FRONTEND::NotSupportedException("setTunerReferenceSource not supported for individual channels. Use device property interface instead.");
}
long USRP_UHD_i::getTunerReferenceSource(const std::string& allocation_id){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].reference_source;
}
void USRP_UHD_i::setTunerEnable(const std::string& allocation_id, bool enable) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);

    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx)){
        LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: ID (" << allocation_id << ") DOES NOT HAVE AUTHORIZATION TO MODIFY TUNER!" );
        throw FRONTEND::FrontendException((std::string(__PRETTY_FUNCTION__)+" - ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    }

    size_t usrp_channel_number=-1;
    try {
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
        exclusive_lock lock(prop_lock);
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

        usrp_channel_number = device_channels[idx].chan_num;

        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
    } catch (...) {
        LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
        throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
    }

    interrupt(idx);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
    exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

    if(enable)
        usrpEnable(idx,usrp_channel_number);
    else
        usrpDisable(idx,usrp_channel_number);

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);
}
bool USRP_UHD_i::getTunerEnable(const std::string& allocation_id) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    return frontend_tuner_status[idx].enabled;
}

void USRP_UHD_i::setTunerOutputSampleRate(const std::string& allocation_id, double sr) {
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);

    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    if(allocation_id != getControlAllocationId(idx)){
        LOG_WARN(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: ID (" << allocation_id << ") DOES NOT HAVE AUTHORIZATION TO MODIFY TUNER!" );
        LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
        throw FRONTEND::FrontendException((std::string(__PRETTY_FUNCTION__)+" - ID "+allocation_id+" does not have authorization to modify the tuner").c_str());
    }
    try {
        size_t usrp_channel_number=-1;
        double opt_sr = sr;
        try {
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
            exclusive_lock lock(prop_lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

            if (frontend::compareHz(sr,0.0) < 0 ||
                    frontend::compareHz(sr,device_channels[idx].rate_max) > 0 ){
                std::ostringstream msg;
                msg << __PRETTY_FUNCTION__ << ":: INVALID SAMPLE RATE (" << sr <<")";
                LOG_WARN(USRP_UHD_i,msg.str() );
                LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
                throw FRONTEND::BadParameterException(msg.str().c_str());
            }

            opt_sr = optimizeRate(sr, idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "REQ_SR=" << sr << " OPT_SR=" << opt_sr);
            usrp_channel_number = device_channels[idx].chan_num;
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
        } catch (FRONTEND::BadParameterException) {
            throw;
        } catch (...) {
            LOG_ERROR(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " Could not retrieve tuner_id to usrp channel number mapping" );
            throw FRONTEND::FrontendException("ERROR: Could not retrieve tuner_id to usrp channel number mapping");
        }

        if (frontend_tuner_status[idx].tuner_type == "RX_DIGITIZER") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // set hw with new value
            usrp_device_ptr->set_rx_rate(opt_sr, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].sample_rate = usrp_device_ptr->get_rx_rate(usrp_channel_number);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "REQ_SR=" << sr << " OPT_SR=" << opt_sr << " TUNER_SR=" << frontend_tuner_status[idx].sample_rate);
            usrp_tuners[idx].update_sri = true;

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else if (frontend_tuner_status[idx].tuner_type == "TX") {

            interrupt(idx);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for tuner lock for tuner_id " << idx);
            exclusive_lock tuner_lock(*usrp_tuners[idx].lock);
            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired tuner lock for tuner_id " << idx);

            // set hw with new value
            usrp_device_ptr->set_tx_rate(opt_sr, usrp_channel_number);

            // update status from hw
            frontend_tuner_status[idx].sample_rate = usrp_device_ptr->get_tx_rate(usrp_channel_number);

            LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing tuner lock for tuner_id" << idx);

        } else {
            LOG_ERROR(USRP_UHD_i,__PRETTY_FUNCTION__ << " :: INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!");
            throw FRONTEND::BadParameterException((std::string(__PRETTY_FUNCTION__)+" : INVALID TUNER TYPE. MUST BE RX_DIGITIZER OR TX!").c_str());
        }

    } catch (std::exception& e) {
        LOG_WARN(USRP_UHD_i,std::string(__PRETTY_FUNCTION__) + " EXCEPTION:: " + std::string(e.what()) );
        throw FRONTEND::FrontendException(("WARNING: " + std::string(e.what())).c_str());
    }

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "waiting for prop_lock");
    exclusive_lock lock(prop_lock);
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "acquired prop_lock");

    updateDeviceInfo();

    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << "releasing prop_lock");
}
double USRP_UHD_i::getTunerOutputSampleRate(const std::string& allocation_id){
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__);
    long idx = getTunerMapping(allocation_id);
    if (idx < 0) throw FRONTEND::BadParameterException("Invalid allocation id");
    LOG_DEBUG(USRP_UHD_i,__PRETTY_FUNCTION__ << " TUNER_SR=" << frontend_tuner_status[idx].sample_rate);
    return frontend_tuner_status[idx].sample_rate;
}
