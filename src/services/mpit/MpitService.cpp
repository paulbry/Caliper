// LLNL-CODE-678900
// All rights reserved.
//
// For details, see https://github.com/scalability-llnl/Caliper.
// Please also see the LICENSE file for our additional BSD notice.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the disclaimer below.
//  * Redistributions in binary form must reproduce the above copyright notice, this list // //    conditions and the disclaimer (as noted below) in the documentation and/or other materials
//    conditions and the disclaimer (as noted below) in the documentation and/or other materials
//    provided with the distribution.
//  * Neither the name of the LLNS/LLNL nor the names of its contributors may be used to endorse
//    or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// LAWRENCE LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
/// @file MpitService.cpp
/// @brief Caliper MPIT service
#include "../CaliperService.h"

#include "caliper/Caliper.h"

#include "caliper/common/Log.h"
#include "caliper/common/RuntimeConfig.h"

#include <mpi.h>

#include <vector>

using namespace cali;
using namespace std;

#define NAME_LEN 1024

namespace
{
    vector<Attribute> mpit_pvar_attr   { Attribute::invalid };
	vector<MPI_T_pvar_handle> pvar_handles;
	vector<int> pvar_count;

    bool      mpit_enabled  { false };

	ConfigSet        config;

	ConfigSet::Entry configdata[] = {
    	ConfigSet::Terminator
	};

	MPI_T_pvar_session pvar_session;
	int num_pvars = 0;

	/*Allocate handles for pvars*/
	void mpit_allocate_pvar_handles() {
		int current_num_pvars, return_val;
		char pvar_name[NAME_LEN], pvar_desc[NAME_LEN];
		int var_class, verbosity, bind, readonly, continuous, atomic, name_len, desc_len;
		MPI_Datatype datatype;
		MPI_T_enum enumtype;

		desc_len = name_len = NAME_LEN;
		/* Get the number of pvars exported by the implementation */
		return_val = MPI_T_pvar_get_num(&current_num_pvars);
		if (return_val != MPI_SUCCESS)
		{
			perror("MPI_T_pvar_get_num ERROR:");
			Log(0).stream() << "MPI_T_pvar_get_num ERROR: " << return_val << endl;
		    return;
		}

		pvar_handles.reserve(current_num_pvars);
		Log(0).stream() << "Num PVARs exported: " << current_num_pvars << endl;

		for(int index=num_pvars; index < current_num_pvars; index++) {
			MPI_T_pvar_get_info(index, pvar_name, &name_len, &verbosity, &var_class, &datatype, &enumtype, 
								pvar_desc, &desc_len, &bind, &readonly, &continuous, &atomic);
			
			/* allocate a pvar handle that will be used later */
			return_val = MPI_T_pvar_handle_alloc(mpit_pvar_session, index, NULL, &pvar_handles[index], &pvar_count[index]);
			if (return_val != MPI_SUCCESS)
			{
				Log(0).stream() << "MPI_T_pvar_handle_alloc ERROR:" << return_val << " for PVAR at index " << index << " with name " << pvar_name << endl;
		    	return;
  			}

			Log(0).stream() << "PVAR at index " << index << " has name: " << pvar_name << endl;
		}
		::num_pvars = current_num_pvars;
	}


	/*Register the service and initalize the MPI-T interface*/
	void mpit_register(Caliper *c)
	{	
	    int thread_provided, return_val;

    	config = RuntimeConfig::init("mpit", configdata);
    
		/* Initialize MPI_T */
		return_val = MPI_T_init_thread(MPI_THREAD_SINGLE, &thread_provided);

		if (return_val != MPI_SUCCESS) 
		{
			Log(0).stream() << "MPI_T_init_thread ERROR: " << return_val << ". MPIT service disabled." << endl;
			return;
		} 
		else 
		{
		/* Track a performance pvar session */
			return_val = MPI_T_pvar_session_create(&pvar_session);
			if (return_val != MPI_SUCCESS) {
			    Log(0).stream() << "MPI_T_pvar_session_create ERROR: " << return_val << ". MPIT service disabled." << endl;
			    return;
		    }  
 		}
    	
		mpit_enabled = true; 
    
    	Log(1).stream() << "Registered MPIT service" << endl;
		
		mpit_allocate_pvar_handles();
	}

} // anonymous namespace 

namespace cali 
{
    CaliperService mpit_service = { "mpit", ::mpit_register };
} // namespace cali
