/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pthread.h>

#include <gnuradio/io_signature.h>
#include <gnuradio/blocks/control_loop.h>
#include <gnuradio/expj.h>
#include <gnuradio/sincos.h>
#include <gnuradio/math.h>
#include "gps_despread_impl.h"

#define DEBUG_OUT


namespace gr {
  namespace gps {

    gps_despread::sptr
    gps_despread::make(int osr_in, float loop_bw)
    {
      return gnuradio::get_initial_sptr
        (new gps_despread_impl(osr_in, loop_bw));
    }

    /*
     * The private constructor
     */
    gps_despread_impl::gps_despread_impl(int osr_in, float loop_bw)
      : gr::sync_decimator("gps_despread",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)), 1023*osr_in)
    {
		// reset delay counter
		delay_selection = 0;
			
		// calcualte some code lut
		set_code(1);
		code_counter = 0;
		
		// default to no osr
		osr_int = osr_in;
		osr_counter = 1;
		
		// reset search and tracking params
		freq_track_step = 2 * M_PI * 100 / (1023e3 * osr_int);
		freq_search_Nsteps = 21;
		int i;
		for(i = 0;i < 4;++i)
		{
			nco_freq_track[i] = 0;
			nco_phase_track[i] = 0;
			freq_track_integrator[i] = 0;
			code_track_integrator[i] = 0;
		}
		freq_track_integrator[4] = 0;
		code_track_integrator[4] = 0;
		
		data.osr = osr_int;

		data.freq_search_Nsteps = freq_search_Nsteps;
		data.buffer = 0;
		data.best_freq = 0;
		data.best_delay = 0;
		data.best_power = -1;
		data.running = 0;
		data.started = 0;
		
		search_enabled = 0;
		search_avg_selection = 0;
		
		// frequency tracking loop
		nco_freq = 0;
		nco_phase = 0;



#ifdef DEBUG_OUT

#endif
    }
	
	

    /*
     * Our virtual destructor.
     */
    gps_despread_impl::~gps_despread_impl()
    {
#ifdef DEBUG_OUT
		
#endif
    }
	
	int gps_despread_impl::search_running() const
	{
		return search_enabled;
	}
	
	void gps_despread_impl::start_search(int avg)
	{
		if(search_enabled)
		{
#ifdef DEBUG_OUT
			printf("### ERROR: Search already running \n");
#endif
			return;
		}
		
		search_buffer_fill_counter = 0;
		
		search_avg_selection = avg;
		
		set_delay(0);
		
		// allocate buffer for searching
		buffer = new gr_complex[1023 * osr_int * search_avg_selection];
		
		if(buffer == nullptr)
		{
			printf("### ERROR: malloc returned -1\n");
			search_enabled = 0;
		}
		

		
#ifdef DEBUG_OUT
		printf("### Search started \n");
		
#endif
		search_enabled = 1;
	}
	
	
	
	void gps_despread_impl::set_delay(int d)
	{
		
		
		delay_selection = d;
		code_changed = 1;

		printf("# Delay changed to %d\n", d);
	}
	
	int gps_despread_impl::delay() const
	{
		
		return delay_selection;
	}
	
	int gps_despread_impl::osr() const
	{
		return osr_int;
		
	}
	
	void gps_despread_impl::set_osr(int o)
	{
		//this->osr_int = o;
		
	}
	
	int gps_despread_impl::code() const 
	{
		return code_selection;
		
	}

	void gps_despread_impl::set_code(int c)
	{
		int g1_tap0 = 0;
		int g1_tap1 = 0;
		
		code_selection = c;
		
		printf("### Set code to: %d\n", c);
		// code generator and tap coefficients from 
		// Matjaz Vidmar
		// http://lea.hamradio.si/~s53mv/navsats/theory.html
		
		switch(c) 
		{
			case 1:
			g1_tap0=1;
			g1_tap1=5;
			break;
			
			case 2:
			g1_tap0=2;
			g1_tap1=6;
			break;
			
			case 3:
			g1_tap0=3;
			g1_tap1=7;
			break;
			
			case 4:
			g1_tap0=4;
			g1_tap1=8;
			break;
			
			case 5:
			g1_tap0=0;
			g1_tap1=8;
			break;
			
			case 6:
			g1_tap0=1;
			g1_tap1=5;
			break;
			
			case 7:
			g1_tap0=0;
			g1_tap1=7;
			break;
			
			case 8:
			g1_tap0=1;
			g1_tap1=8;
			break;
			
			case 9:
			g1_tap0=2;
			g1_tap1=9;
			break;
			
			case 10:
			g1_tap0=1;
			g1_tap1=2;
			break;
			
			case 11:
			g1_tap0=2;
			g1_tap1=3;
			break;
			
			case 12:
			g1_tap0=4;
			g1_tap1=5;
			break;
			
			case 13:
			g1_tap0=5;
			g1_tap1=6;
			break;
			
			case 14:
			g1_tap0=6;
			g1_tap1=7;
			break;
			
			case 15:
			g1_tap0=7;
			g1_tap1=8;
			break;
			
			case 16:
			g1_tap0=8;
			g1_tap1=9;
			break;
			
			case 17:
			g1_tap0=0;
			g1_tap1=3;
			break;
			
			case 18:
			g1_tap0=1;
			g1_tap1=4;
			break;
			
			case 19:
			g1_tap0=2;
			g1_tap1=5;
			break;
			
			case 20:
			g1_tap0=3;
			g1_tap1=6;
			break;
			
			case 21:
			g1_tap0=4;
			g1_tap1=7;
			break;
			
			case 22:
			g1_tap0=5;
			g1_tap1=8;
			break;
			
			case 23:
			g1_tap0=0;
			g1_tap1=2;
			break;
			
			case 24:
			g1_tap0=3;
			g1_tap1=5;
			break;
			
			case 25:
			g1_tap0=4;
			g1_tap1=6;
			break;
			
			case 26:
			g1_tap0=5;
			g1_tap1=7;
			break;
			
			case 27:
			g1_tap0=6;
			g1_tap1=8;
			break;
			
			case 28:
			g1_tap0=7;
			g1_tap1=9;
			break;
			
			case 29:
			g1_tap0=0;
			g1_tap1=5;
			break;
			
			case 30:
			g1_tap0=1;
			g1_tap1=6;
			break;
			
			case 31:
			g1_tap0=2;
			g1_tap1=7;
			break;
			
			case 32:
			g1_tap0=3;
			g1_tap1=8;
			break;
	
			default: // default
			code_selection=1;
			g1_tap0=1;
			g1_tap1=5;
		}
		
		printf("tap0: %d, tap2: %d\n", g1_tap0, g1_tap1);

		// calculate our code LUT
		
		char g0[10] = {1,1,1,1,1,1,1,1,1,1};
		char g1[10] = {1,1,1,1,1,1,1,1,1,1};
		char g1_out = 0;
		
		
		int i = 0, j = 0;
		for(i = 0 ; i < 1023 ; ++i)
		{
			g1_out = g1[g1_tap0] ^ g1[g1_tap1];
			
			
			code_LUT[i] = (gr_complex) (2.0 * ((g1_out ^ g0[9]) - 0.5));
			//printf("%d ", g1_out ^ g0[9]);
			
			char g0_state0_next = g0[2] ^ g0[9];
			char g1_state0_next = g1[1] ^ g1[2] ^ g1[5]
					 			^ g1[7] ^ g1[8] ^ g1[9];
			
			for(j = 9; j >= 0; --j)
			{
				if(j == 0)
				{
					g0[j] = g0_state0_next;
					g1[j] = g1_state0_next;
				}
				else
				{
					g0[j] = g0[j-1];
					g1[j] = g1[j-1];
				}
			}
			
			
			
		}
		
		code_changed = 1;
		//printf("\n");
		
	}
	
	float gps_despread_impl::error() const
	{
		return 0;
	}
	
	int gps_despread_impl::find_max_code() const
	{
		float best = 0;
		int best_ix = 0;
		int i;
		
		for(i = 0; i < 5; ++i)
		{
			if(abs(freq_track_integrator[i]) > best)
			{
				best = abs(freq_track_integrator[i]);
				best_ix = i;
			}
		}
		
		return best_ix;
	}
	
	int gps_despread_impl::find_max_freq() const
	{
		float best = 0;
		int best_ix = 0;
		int i;
		
		for(i = 0; i < 5; ++i)
		{
			if(abs(freq_track_integrator[i]) > best)
			{
				best = abs(freq_track_integrator[i]);
				best_ix = i;
			}
		}
		
		return best_ix;
	}

    int
    gps_despread_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
		unsigned long i = 0;
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];
		
		increment_counter = 0;
		
		int noutputs = 0;
		
		out[noutputs] = 0; // reset integrator

		for(i = 0; i < noutput_items*1023*osr_int; ++i)
		{
			int code_sel = ((code_counter + delay_selection)/osr_int) % 1023;
			
			if (code_sel < 0)
				code_sel += 1023;
			
			nco_phase = nco_phase + nco_freq;
			
			while(nco_phase > 2*M_PI)
				nco_phase -= 2*M_PI;
			while(nco_phase < -2*M_PI)
				nco_phase += 2*M_PI;
			
			gr_complex nco = gr_expj(-nco_phase);

			gr_complex s = nco * in[i] * code_LUT[code_sel];
			out[noutputs] += s / (float)osr_int ;
			increment_counter++;
			
			// tracking integrators
			int j;
			for(j = 0; j < 4; ++j)
			{
				nco_phase_track[j] = nco_phase_track[j] + nco_freq_track[j];
				
				while(nco_phase_track[j] > 2*M_PI)
					nco_phase_track[j] -= 2*M_PI;
				while(nco_phase_track[j] < -2*M_PI)
					nco_phase_track[j] += 2*M_PI;
				
				gr_complex nco_track = gr_expj(-nco_phase_track[j]);
				
				freq_track_integrator[j+1] += in[i] * code_LUT[code_sel] * nco_track;
			}
			
			
			for(j = 0; j < 4; ++j)
			{
				if(j < 2)
				{
					int code_sel_track = code_sel - j - 1;
					if (code_sel_track < 0)
						code_sel_track += 1023 * osr_int;
				
					code_track_integrator[j+1] += in[i] * code_LUT[code_sel_track] * nco;
				}
				else
				{
					int input_sel_track = i - j + 2;
				
					if(input_sel_track < 0)
						input_sel_track += 1023 * osr_int;
				
					
					code_track_integrator[j+1] += in[input_sel_track] * code_LUT[code_sel] * nco;
				}
			}
			
			
			if(( (delay_selection + code_counter) % (1023 * osr_int) ) == (1023 * osr_int - 1) )
			{
				// we don't expect there to ever be this big delay jump but
				// this filters out "short sample" caused by delay change from 0 to -1
				if(increment_counter > 1023 * osr_int / 2)
				{
					increment_counter = 0;
					noutputs++;
					out[noutputs] = 0;
				}
				
				// TODO: only allow code delay to be changed at this time instant
				
				
				
				int best_freq_ix;
				int best_delay_ix;
				
				// add current delay/freq to the comparison list
				freq_track_integrator[0] = out[noutputs - 1];
				code_track_integrator[0] = out[noutputs - 1];
					
				best_freq_ix = find_max_freq();
				best_delay_ix = find_max_code();
				
				// check if we have drifted a fraction of chip or if doppler has changed
				if(best_freq_ix != 0)
				{
					float adjustment = 2 * M_PI * 1 / (1023e3 * osr_int);
					
					if(best_freq_ix < 2)
						nco_freq = nco_freq - adjustment;
					else
						nco_freq = nco_freq + adjustment;
					//nco_freq = nco_freq_track[best_freq_ix-1];
					
					// 1Hz adjustment step allows doppler to change by 1kHz in a second
					
					// move tracking frequencies to match new center
					for(j = 0;j < 2; ++j)
						nco_freq_track[j] = nco_freq - freq_track_step * j;
					for(j = 2;j < 4; ++j)
							nco_freq_track[j] = nco_freq + freq_track_step * (j - 2);
					
					//if(best_freq_ix < 3)
					//	printf("freq: %d (%f)\n", best_freq_ix - 3,nco_freq / 2.0 / M_PI * 1023e3 * osr_int);
					//else
					//	printf("freq: %d\n", best_freq_ix - 2,nco_freq / 2.0 / M_PI * 1023e3 * osr_int);
				}
				else
					printf("freq: %d (%f) (power=%f)\n", best_freq_ix - 3,nco_freq / 2.0 / M_PI * 1023e3 * osr_int, abs(freq_track_integrator[best_freq_ix]));
				
				if(best_delay_ix != 0)
				{
					if(best_delay_ix < 3)
						set_delay(delay_selection + (best_delay_ix -1 -2));
					else
						set_delay(delay_selection + (best_delay_ix -2));
					
					//if(best_delay_ix < 3)
					//	printf("delay: %d\n", best_delay_ix - 3);
					//else
					//	printf("delay: %d\n", best_delay_ix - 2);
				}
					//printf("delay: 0\n");
				
				// resetthe integrators
				for(j = 0;j < 5;++j)
				{
					freq_track_integrator[j] = 0;
					code_track_integrator[j] = 0;
				}
			}
			
			if(search_enabled)
			{
				
				// synchronize internal counter to data by waiting (lol)
				if(code_changed && code_sel == 0)
				{	
					code_changed = 0;
				}

				
				if(!code_changed)
				{
					// signal is converted to zero IF for search
					buffer[search_buffer_fill_counter++] = in[i];		
				}
				
				if( search_buffer_fill_counter == 1023*osr_int*search_avg_selection ) 
				{
					search_enabled = 0;
	 			    // fill in input data for search thread
					int k;
					for(k = 0; k < 1023 ; ++k)
					{
						
						data.spreading_code[k] = code_LUT[k];
										
					}
					
					
					data.buffer = buffer;
					data.len = 1023*osr_int*search_avg_selection;
					
					
					// start search thread
					pthread_create(&search_thread, NULL, gr::gps::gps_despread_impl::search, &data);
				}


			}
			
			if(data.started && !data.running)
			{
				// search thread has finished
				printf("# Setting delay=%d , freq=%f", data.best_delay, data.best_freq);
				nco_freq = 2 * M_PI * data.best_freq / (1023e3 * osr_int);
				set_delay(data.best_delay);
				
				for(j = 0;j < 2; ++j)
					nco_freq_track[j] = nco_freq - freq_track_step * j;
				for(j = 2;j < 4; ++j)
					nco_freq_track[j] = nco_freq + freq_track_step * (j - 2);
				
				data.started = 0;
			}
			
			
			if(osr_counter == osr_int)
			{
				osr_counter = 1;
			}
			else
				osr_counter++;
			
			if(code_counter == (1023 * osr_int) - 1)
			{
				code_counter = 0;					
			}
			else
				code_counter++;
			//printf("%d consumed: %d \n",code_counter, consumed);

		}
        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace gps */
} /* namespace gr */

