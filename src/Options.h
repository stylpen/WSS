/*
 * Options.h
 *
 *  Created on: Oct 2, 2013
 *      Author: stephan
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

struct Options{
	std::string broker_hostname;
	std::string broker_port;
	std::string broker_ca;
	std::string broker_tls_version;
	bool broker_allow_self_signed_certificates;
	bool broker_tls;
	std::string ws_crt;
	std::string ws_key;
	std::string ws_dh;
	std::string ws_tls_version;
	bool ws_tls;
};

#endif /* OPTIONS_H_ */
