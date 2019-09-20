
/* -*- c++ -*- */
/* 
 * Copyright 2019 GPL.
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

#ifndef INCLUDED_SFE_DEVICE_H
#define INCLUDED_SFE_DEVICE_H

namespace gr {
  namespace simplefe {


	  class sfe_device 
	  {
	  private:
		  sfe_device()
		  {
			  m_sfe = sfe_init();
			  sfe_reset_board(m_sfe);
		  }
		  ~sfe_device()
		  {
			  sfe_close(m_sfe);
		  }

		  static sfe_device* m_gb_dev;
		  sfe* m_sfe;
	  public:
		  static sfe_device* get_device() {
			  if (!m_gb_dev) {
				  m_gb_dev = new sfe_device();
			  }
			  return m_gb_dev;
		  }

		  sfe* dev() {
			  return m_sfe;
		  }
	  };
  }
}

#endif
