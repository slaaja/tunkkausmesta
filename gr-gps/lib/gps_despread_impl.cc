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

	void max_abs(gr_complex *b, int l, float &a, int &ix, float &avg)
	{
		unsigned long i = 0;

		avg = 0;
		a = 0;
		ix = 0;
	
		for(i = 0;i < l; ++i)
		{
			avg += abs(b[i]);	
			if(abs(b[i]) > a)
			{
				a = abs(b[i]);
				ix = i;
			}
		}
	
		avg = avg / l;
	
	}

	void calculate_product(gr_complex *in, gr_complex *c, gr_complex *out, int l, int shift )
	{
		int i = 0;
	
		for(i = 0; i < l; ++i)
		{
			int code_ix = i - shift;
		
			while(code_ix < 0)
			{
				code_ix = code_ix + l;
			}
		
			while(code_ix >= l)
			{
				code_ix = code_ix - l;
			}
		
			out[i] = in[i] * c[code_ix];		
		}
	
	}

    gps_despread::sptr
    gps_despread::make(int code_sel, int osr_in)
    {
      return gnuradio::get_initial_sptr
        (new gps_despread_impl(code_sel, osr_in));
    }

    /*
     * The private constructor
     */
    gps_despread_impl::gps_despread_impl(int code_sel, int osr_in)
      : gr::block("gps_despread",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(gr_complex)) )
    {
		// reset delay counter
		delay_selection = 0;
			
		// fill the code lut
		generate_codes();
		set_code(code_sel);

		code_counter = 0;
		
		osr_int = osr_in;

		// reset search and tracking params
		freq_search_Nsteps = 21;
		int i;
		
		// state machine
		fsm = state_search;

		// code tracking		
		for(i = 0; i < 5 ; ++i)
			track_integrator[i] = 0;
		track_counter = 0;
		
		// fft objects for search

		fft_c = new gr::fft::fft_complex(1023 * osr_int, 1, 4);
		ifft_c = new gr::fft::fft_complex(1023 * osr_int, 0, 4);		


		// buffers for FFT fine frequency correction
		freq_corr_integrator_i = 0;
		freq_corr_integrator_q = 0;
		freq_corr_integrator_i_d = 0;
		freq_corr_integrator_q_d = 0;

		search_avg_selection = 0;
		
		// frequency tracking loop
		nco_freq = 0;
		lf_int = 0;
		lf_zero_d = 0;
		lf_pole_d = 0;
		
		nco_phase = 0;
    }
	
	

    /*
     * Our virtual destructor.
     */
    gps_despread_impl::~gps_despread_impl()
    {

		delete ifft_c;
		delete fft_c;
    }
	
	
	void gps_despread_impl::set_delay(int d)
	{

		if(d >= 0)
			delay_selection = d % (1023 * osr_int);
		else
			delay_selection = osr_int * 1023 + (d % (1023 * osr_int));

		printf("## Delay changed to %d\n", delay_selection);
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
	
	void gps_despread_impl::set_freq(float f)
	{
		printf("## NCO frequency fixed part set to: %f\n", f);
		nco_freq_fixed = 2 * M_PI * f / (1023e3 * osr_int);
	}

	int gps_despread_impl::code() const 
	{
		return code_selection;
		
	}

	void gps_despread_impl::set_code(int c)
	{
		if(c < 32 && c > 0)
		{
			printf("## Code set to: %d\n", c);
			code_selection = c;
		}
		else
			printf("ERROR: Unknown code selection: %d\n", c);
	}

		


	void gps_despread_impl::search(const gr_complex *in)
	{
		int best_delay = 0;
		float best_power = -1;
		float best_freq = 0;

		int data_len = 1023 * osr_int;

		int d = 0;
		int f = 0;
		int i;

		printf("Searching...\n");

		gr_complex *input_buffer = fft_c->get_inbuf();
		gr_complex *output_buffer = fft_c->get_outbuf();
			
		//
		// calculate fft of input data and spreading code
		//
		gr_complex *code_fft_conj = new gr_complex[data_len];
		gr_complex *input_data_fft = new gr_complex[data_len];
			
		for(i = 0; i < data_len; ++i)
		{
			input_buffer[i] = code_LUT[((int)(i/osr_int)) % 1023][code_selection - 1];
		}

		fft_c->execute();
			
		for(i = 0; i < data_len; ++i)
		{
			code_fft_conj[i] = conj(output_buffer[i]);
			input_buffer[i] = in[i];
		}

			
		fft_c->execute();
			
		for(i = 0; i < data_len ; ++i)
		{
			input_data_fft[i] = output_buffer[i];
		}
			
			
		float freq_step = 1000.0f;

		
	
		input_buffer = ifft_c->get_inbuf();
		output_buffer = ifft_c->get_outbuf();

		float peak;

		for(i = -(freq_search_Nsteps >> 1) ; i < (freq_search_Nsteps >> 1) ; ++i)
		{
			calculate_product(input_data_fft, code_fft_conj, input_buffer, data_len, i);
					
			ifft_c->execute();
					
			int peak_offset;
			float avg;
					
			max_abs(output_buffer, data_len, peak, peak_offset, avg);
					
			//printf("i: %d, peak: %f , peak_offset: %d\n", i, peak, peak_offset);
			if(peak > best_power)
			{				
				best_power = peak;
				best_freq = i * freq_step;
				best_delay = peak_offset;
			}
		}


		float threshold = 0;
		if( peak > threshold)
		{
			set_freq(best_freq);


			set_delay(osr_int * 1023 - best_delay);
			
			// go to track mode
			fsm = state_track;
		}


		printf("search:: consumed: %d\n", data_len);
		consume_each(data_len);
		
		delete code_fft_conj;
		delete input_data_fft;

	}

	void
	gps_despread_impl::forecast(int noutput_items, gr_vector_int &ninput_items_required)
	{
		unsigned ninputs = ninput_items_required.size();

		// we need at least a whole sequence
		for(int i = 0; i < ninputs; ++i)
		{
			ninput_items_required[i] = noutput_items * 1023 * osr_int;
		}
	}

	void
	gps_despread_impl::update_pll(float freqerror)
	{
		float freqerror_trunc = freqerror; 

		while(freqerror_trunc > M_PI/2)
			freqerror_trunc -= M_PI;
		while(freqerror_trunc < -M_PI/2)
			freqerror_trunc += M_PI;

		phase_error += freqerror_trunc;

		// two pole, one zero loop filter
		float Kp = 0.905082;
		float Kz = 0.990143;

		lf_int = lf_int + (lf_pole_d - lf_zero_d + phase_error)/512.0f / 2000.0;
		lf_pole_d =  Kp * (lf_pole_d - lf_zero_d + phase_error);		 
		lf_zero_d = Kz * phase_error;

		nco_freq = lf_int;


		FILE *fid = fopen("/home/samu/testi_out.txt", "a+");
		fprintf(fid, "%f,\n", (nco_freq + nco_freq_fixed) * 1023e3 * osr_int / 2 / M_PI);
		fclose(fid);

		printf("nco_freq: %f\n", (nco_freq + nco_freq_fixed) * 1023e3 * osr_int / 2 / M_PI);
	}

	void
	gps_despread_impl::track(const gr_complex *in, gr_complex *out, int noutput_items, int &noutputs)
	{
		int consumed;
		noutputs = 0;

		for(int i = 0; i < noutput_items*1023*osr_int; ++i)
		{
			int code_index = ((code_counter + delay_selection) / osr_int) % 1023;
			
			if (code_index < 0)
				code_index += 1023;
			
			nco_phase = nco_phase + nco_freq + nco_freq_fixed;
			
			while(nco_phase > 2*M_PI)
				nco_phase -= 2*M_PI;
			while(nco_phase < -2*M_PI)
				nco_phase += 2*M_PI;
			
			gr_complex nco = gr_expj(-nco_phase);

			gr_complex s = nco * in[i] * code_LUT[code_index][code_selection - 1];
			out[noutputs] += s / (float)osr_int ;
			
			// code rate correction
			for(int j = -2; j < 3 ; ++j)
			{
				int code_index_track = ((code_counter + delay_selection + j) / osr_int) % 1023;

				if(code_index_track < 0)
					code_index_track += 1023;
					

				track_integrator[j + 2] += nco * in[i] * code_LUT[code_index_track][code_selection - 1]; 
			}
		
			// LO freq error correction
			freq_corr_integrator_i += nco * in[i] * code_LUT[code_index][code_selection -1];
			freq_corr_integrator_q += nco * in[i] * code_LUT[code_index][code_selection -1];
			
			if( (code_counter + delay_selection) == (osr_int * 1023 / 2 - 1) )
			{	
				float freq_error = (arg(freq_corr_integrator_q) - arg(freq_corr_integrator_i_d) );
				
				update_pll(freq_error);

				freq_corr_integrator_q_d = freq_corr_integrator_q;
				freq_corr_integrator_q = 0;
			}
				
			// code wrapped around -> sample ready
			if( (code_counter + delay_selection) == (osr_int * 1023 - 1) )
			{
				noutputs++;

				out[noutputs] = 0;


				// freq tracking
				float freq_error = (arg(freq_corr_integrator_i) - arg(freq_corr_integrator_q_d) );
				update_pll(freq_error);
				

				freq_corr_integrator_i_d = freq_corr_integrator_i;
				freq_corr_integrator_i = 0;

				// code tracking
				if(track_counter == 1)
				{
					int best_ix;
					float best_power;
					float avg;

					max_abs(track_integrator, 5, best_power, best_ix, avg);

					printf("code track:: [ ");
					for(int j = 0; j < 5 ; ++j)
						printf("%5.1f ", abs(track_integrator[j]));
					printf("]\n");

					set_delay(delay_selection + best_ix - 2);

					for(int j = 0; j < 5 ; ++j)
						track_integrator[j] = 0; 

					track_counter = 0;
				}
				else
				{
					track_counter++;
				}
			}		
			
			// This counter is our reference point for the delay selection.
			//
			// Even though search consumes the samples it uses for searching,
			// this counter remains in sync because the number of
			// samples used for search is always exactly one full rotation
			// of this counter.		
			if(code_counter == (1023 * osr_int) - 1)
			{
				code_counter = 0;
			}
			else
				code_counter++;

			consumed++;
		}

		consume_each(consumed);
	}

    int
    gps_despread_impl::general_work(int noutput_items, 
			  gr_vector_int &ninput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
		unsigned long i = 0;
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

		int noutputs = 0;
		
		out[noutputs] = 0; // reset integrator

		switch(fsm)
		{
			case state_track:
				track(in, out, noutput_items, noutputs);
				break;
		
			case state_search:
			default:
				search(in);
				break;			
		}
		
        return noutputs;
    }

	
	void gps_despread_impl::generate_codes()
	{
		int g1_tap0 = 0;
		int g1_tap1 = 0;
		int c;

		for(c = 0; c < 32 ; ++c)
		{		
			switch(c + 1) 
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
			}
		

			// calculate our code LUT
		
			char g0[10] = {1,1,1,1,1,1,1,1,1,1};
			char g1[10] = {1,1,1,1,1,1,1,1,1,1};
			char g1_out = 0;
		
		
			int i = 0, j = 0;
			for(i = 0 ; i < 1023 ; ++i)
			{
				g1_out = g1[g1_tap0] ^ g1[g1_tap1];
			
			
				code_LUT[i][c] = (gr_complex) (2.0 * ((g1_out ^ g0[9]) - 0.5));
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
		}

	}

  } /* namespace gps */
} /* namespace gr */

