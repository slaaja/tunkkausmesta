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

#include <gnuradio/io_signature.h>
#include "vector_average_vcc_impl.h"

namespace gr {
  namespace gps {

    vector_average_vcc::sptr
    vector_average_vcc::make(int vector_size, int num_avg)
    {
      return gnuradio::get_initial_sptr
        (new vector_average_vcc_impl(vector_size, num_avg));
    }

    /*
     * The private constructor
     */
    vector_average_vcc_impl::vector_average_vcc_impl(int vector_size, int num_avg)
      : gr::sync_decimator("vector_average_vcc",
              gr::io_signature::make(1, 1, sizeof(gr_complex) * vector_size),
              gr::io_signature::make(1, 1, sizeof(gr_complex) * vector_size), num_avg)
    {
		vector_size_int = vector_size;
		num_avg_int = num_avg;
	}

    /*
     * Our virtual destructor.
     */
    vector_average_vcc_impl::~vector_average_vcc_impl()
    {
    }

    int
    vector_average_vcc_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];

        // Do <+signal processing+>
		int count = 0;

		while(count < noutput_items)
		{
			int i = 0;
			int j = 0;

			for(i = 0 ; i < vector_size_int; ++i)
			{

				for(j = 0;j < num_avg_int; ++j)
					out[i + count * vector_size_int] 
						+= in[i + j * num_avg_int + count * num_avg_int * vector_size_int] / (float) num_avg_int;

			}

			count++;
		}


        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace gps */
} /* namespace gr */

