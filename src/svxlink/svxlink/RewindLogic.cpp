/**
@file	 RewindLogic.cpp
@brief   A class to connect a brandmeiseter server with UDP
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-02-12

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <string>
#include <ctime>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/


#include <AsyncUdpSocket.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "RewindLogic.h"
#include "sha256.h"
#include "version/SVXLINK.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

RewindLogic::RewindLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name),
    m_state(DISCONNECTED), m_udp_sock(0), m_auth_key("passw0rd"),
    rewind_host(""), rewind_port(54005), m_callsign("N0CALL"),
    m_ping_timer(5000, Timer::TYPE_PERIODIC, false),
    sequenceNumber(0), m_slot1(false), m_slot2(false)
{
} /* RewindLogic::RewindLogic */


RewindLogic::~RewindLogic(void)
{
  delete m_udp_sock;
  m_ping_timer = 0;
  delete dns;
  delete m_logic_con_in;
  delete m_logic_con_out;
} /* RewindLogic::~RewindLogic */


bool RewindLogic::initialize(void)
{
  if (!cfg().getValue(name(), "HOST", rewind_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration"
         << endl;
    return false;
  }

  cfg().getValue(name(), "PORT", rewind_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "ID", m_id))
  {
    cerr << "*** ERROR: " << name() << "/ID missing in configuration"
         << endl;
    return false;
  }

  if (m_id.length() < 6 || m_id.length() > 7)
  {
    cerr << "*** ERROR: " << name() << "/ID is wrong, must have 6 or 7 digits,"
         << "e.g. ID=2620001" << endl;
    return false;
  }

  if (!cfg().getValue(name(), "AUTH_KEY", m_auth_key))
  {
    cerr << "*** ERROR: " << name() << "/AUTH_KEY missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "RX_FREQU", m_rxfreq))
  {
    cerr << "*** ERROR: " << name() << "/RX_FREQU missing in configuration"
         << ", e.g. RX_FREQU=430500000"  << endl;
    return false;
  }
  if (m_rxfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/RX_FREQU wrong length: " << m_rxfreq
         << ", e.g. RX_FREQU=430500000" << endl;
    return false;
  }

  if (!cfg().getValue(name(), "TX_FREQU", m_txfreq))
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU missing in configuration"
         << ", e.g. TX_FREQU=430500000" << endl;
    return false;
  }
  if (m_txfreq.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/TX_FREQU wrong length: " << m_txfreq
         << ", e.g. TX_FREQU=430500000" << endl;
    return false;
  }

  if (!cfg().getValue(name(), "POWER", m_power))
  {
    m_power = "01";
  }
  if (m_power.length() != 2)
  {
    cerr << "*** ERROR: " << name() << "/POWER wrong length: " << m_power
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "COLORCODE", m_color))
  {
    m_color = "01";
  }
  if (m_color.length() != 2 || atoi(m_color.c_str()) <= 0)
  {
    cerr << "*** ERROR: " << name() << "/COLORCODE wrong: " << m_color
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "LATITUDE", m_lat))
  {
    cerr << "*** ERROR: " << name() << "/LATITUDE not set."
         << endl;
    return false;
  }
  if (m_lat.length() != 8)
  {
    cerr << "*** ERROR: " << name() << "/LATITUDE wrong length: "
         << m_lat << endl;
    return false;
  }

  if (!cfg().getValue(name(), "LONGITUDE", m_lon))
  {
    cerr << "*** ERROR: " << name() << "/LONGITUDE not set."
         << endl;
    return false;
  }
  if (m_lon.length() != 9)
  {
    cerr << "*** ERROR: " << name() << "/LONGITUDE wrong length: "
         << m_lon << endl;
    return false;
  }

  if (!cfg().getValue(name(), "HEIGHT", m_height))
  {
    m_height = "001";
  }
  if (m_height.length() != 3)
  {
    cerr << "*** ERROR: " << name() << "/HEIGHT wrong: " << m_height
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "LOCATION", m_location))
  {
    m_location = "none";
  }
  if (m_location.length() > 20)
  {
    cerr << "*** ERROR: " << name() << "/LOCATION to long: " << m_location
         << endl;
    return false;
  }
  while (m_location.length() < 20)
  {
    m_location += ' ';
  }

  if (!cfg().getValue(name(), "DESCRIPTION", m_description))
  {
    m_description = "none";
  }
  if (m_description.length() > 20)
  {
    cerr << "*** ERROR: " << name() << "/DESCRIPTION to long (>20 chars)"
         << endl;
    return false;
  }
    // configure the time slots
  string slot;
  if (cfg().getValue(name(), "SLOT1", slot))
  {
    m_slot1 = true;
  }
  if (cfg().getValue(name(), "SLOT2", slot))
  {
    m_slot2 = true;
  }
  if (cfg().getValue(name(), "TG", m_tg))
  {
    m_tg = "9";
  }

  m_swid += "linux:SvxLink v";
  m_swid += SVXLINK_VERSION;

  m_ping_timer.setEnable(false);
  m_ping_timer.expired.connect(
     mem_fun(*this, &RewindLogic::pingHandler));


  // create the Rewind recoder device, DV3k USB stick or DSD lib
  string m_ambe_handler;
  if (!cfg().getValue(name(), "AMBE_HANDLER", m_ambe_handler))
  {
    cerr << "*** ERROR: " << name() << "/AMBE_HANDLER not valid, must be"
         << " DV3k or SwDsd" << endl;
    return false;
  }

  m_logic_con_in = Async::AudioEncoder::create(m_ambe_handler);
  if (m_logic_con_in == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio encoder" << endl;
    return false;
  }
  m_logic_con_in->writeEncodedSamples.connect(
      mem_fun(*this, &RewindLogic::sendEncodedAudio));
  m_logic_con_in->flushEncodedSamples.connect(
      mem_fun(*this, &RewindLogic::flushEncodedAudio));

    // Create audio decoder
  m_dec = Async::AudioDecoder::create(m_ambe_handler);
  if (m_dec == 0)
  {
    cerr << "*** ERROR: Failed to initialize audio decoder" << endl;
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &RewindLogic::allEncodedSamplesFlushed));
  AudioSource *prev_src = m_dec;

    // Create jitter FIFO if jitter buffer delay > 0
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    AudioFifo *fifo = new Async::AudioFifo(
        2 * jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
        //new Async::AudioJitterFifo(100 * INTERNAL_SAMPLE_RATE / 1000);
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
    prev_src->registerSink(fifo, true);
    prev_src = fifo;
  }
  m_logic_con_out = prev_src;

  // sending options to audio encoder
  string opt_prefix(m_logic_con_in->name());
  opt_prefix += "_";
  list<string> names = cfg().listSection(name());
  list<string>::const_iterator nit;
  for (nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_logic_con_in->setOption(opt_name, opt_value);
      m_dec->setOption(opt_name, opt_value);
    }
  }

  if (!LogicBase::initialize())
  {
    return false;
  }

   // connect the master server
  connect();

  return true;
} /* RewindLogic::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void RewindLogic::connect(void)
{

  if (ip_addr.isEmpty())
  {
    dns = new DnsLookup(rewind_host);
    dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    return;
  }

  cout << name() << ": create UDP socket on port " << rewind_port << endl;

  delete m_udp_sock;
  m_udp_sock = new Async::UdpSocket();
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &RewindLogic::onDataReceived));

   // session initiated by a simple REWIND_TYPE_KEEP_ALIVE message
  sendKeepAlive();

  m_state = CONNECTING;
  m_ping_timer.setEnable(true);

} /* RewindLogic::onConnected */


void RewindLogic::dnsResultsReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();

  delete dns;
  dns = 0;

  if (result.empty() || result[0].isEmpty())
  {
    ip_addr.clear();
    return;
  }

  ip_addr = result[0];
  connect();
} /* RewindLogic::dnsResultsReady */


void RewindLogic::sendEncodedAudio(const void *buf, int count)
{
  //cout << "### " << name() << ": ReflectorLogic::sendEncodedAudio: count="
  //     << count << endl;
/*  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
  sendUdpMsg(MsgUdpAudio(buf, count));*/
} /* RewindLogic::sendEncodedAudio */


void RewindLogic::flushEncodedAudio(void)
{
  //cout << "### " << name() << ": RewindLogic::flushEncodedAudio" << endl;
} /* RewindLogic::sendEncodedAudio */


void RewindLogic::onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  cout << "### " << name() << ": RewindLogic::onDataReceived: addr="
       << addr << " port=" << port << " count=" << count << endl;

  struct RewindData* rd = reinterpret_cast<RewindData*>(buf);

  switch (rd->type)
  {
    case REWIND_TYPE_REPORT:
      cout << "--- server message: " << rd->data << endl;
      return;

    case REWIND_TYPE_CHALLENGE:
      cout << "--- Authentication" << endl;
      authenticate(m_auth_key);
      return;

    case REWIND_TYPE_CLOSE:
      cout << "*** Disconnect request received." << endl;
      m_state = DISCONNECTED;
      m_ping_timer.setEnable(false);
      return;

    case REWIND_TYPE_KEEP_ALIVE:
      if (m_state == WAITING_PASS_ACK)
      {
        cout << "Authenticated!" << endl;
        m_state = AUTHENTICATED;
      }
      sendKeepAlive();
      return;

    case REWIND_TYPE_FAILURE_CODE:
      cout << "*** ERROR: sourceCall or destinationCall could not be "
           << "resolved during processing." << endl;
      return;

    default:
      cout << "*** Unknown data received" << endl;
  }
} /* RewindLogic::udpDatagramReceived */


void RewindLogic::authenticate(const string pass)
{

  struct RewindData* rd =
     (struct RewindData*)alloca(sizeof(struct RewindData) + BUFFER_SIZE);

  memcpy(rd->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);
  rd->type = htole16(REWIND_TYPE_AUTHENTICATION);  // 0x0000 + 3
  rd->flags = htole16(REWIND_FLAG_NONE); //
  rd->length = htole16(SHA256_DIGEST_LENGTH); // 32

  mkSHA256(pass.c_str(), (int)pass.length(), rd->data);

  cout << "Pass=" << pass << " " << rd->data << endl;


  sendMsg(rd, sizeof(struct RewindData) + SHA256_DIGEST_LENGTH);
  m_state = WAITING_PASS_ACK;

} /* RewindLogic::authPassphrase */


void RewindLogic::sendMsg(struct RewindData* data, size_t len)
{

  if (ip_addr.isEmpty())
  {
    if (!dns)
    {
      dns = new DnsLookup(rewind_host);
      dns->resultsReady.connect(mem_fun(*this, &RewindLogic::dnsResultsReady));
    }
    return;
  }

  if (m_udp_sock == 0)
  {
    cerr << "*** ERROR: No Udp socket available" << endl;
    return;
  }

  data->number = htole32(++sequenceNumber);

  cout << "### sending udp packet to " << ip_addr.toString()
       << ":" << rewind_port << ", length=" << len
       << " seq: " << sequenceNumber << " type=" << data->type
       << " inhalt=" << data->number
       << endl;

  m_udp_sock->write(ip_addr, rewind_port, data, len);

} /* RewindLogic::sendUdpMsg */


void RewindLogic::reconnect(Timer *t)
{
  cout << "### Reconnecting to Rewind server\n";
} /* RewindLogic::reconnect */


void RewindLogic::allEncodedSamplesFlushed(void)
{
} /* RewindLogic::allEncodedSamplesFlushed */


void RewindLogic::pingHandler(Async::Timer *t)
{
  // cout << "PING timeout\n";
  sendKeepAlive();
} /* RewindLogic::heartbeatHandler */


void RewindLogic::sendKeepAlive(void)
{
  struct RewindVersionData* vd =
     (struct RewindVersionData*)alloca(sizeof(struct RewindVersionData) + BUFFER_SIZE);
  struct RewindData* rd =
     (struct RewindData*)alloca(sizeof(struct RewindData) + BUFFER_SIZE);

                  // "REWIND01"             8
  memcpy(rd->sign, REWIND_PROTOCOL_SIGN, REWIND_SIGN_LENGTH);

  vd = (struct RewindVersionData*)rd->data;
  vd->number = atoi(m_id.c_str());
  vd->service = REWIND_SERVICE_SIMPLE_APPLICATION; // 0x20 + 0x00
  strcpy(vd->description, m_swid.c_str());

  rd->type = htole16(REWIND_TYPE_KEEP_ALIVE); // 0x00
  rd->flags = htole16(REWIND_FLAG_NONE); // 0x00

  int len = m_swid.length();
  len += sizeof(struct RewindVersionData);
  len += sizeof(struct RewindData);
  rd->length = htole16(len);

  cout << "nummerID=" << vd->number << " laenge " << len << endl;
  sendMsg(rd, len);
  m_ping_timer.reset();

} /* RewindLogic::sendPing */


void RewindLogic::sendServiceData(void)
{
} /* RewindData::sendServiceData */


void RewindLogic::sendCloseMessage(void)
{
  struct RewindData* rd =
     (struct RewindData*)alloca(sizeof(struct RewindData) + BUFFER_SIZE);

  rd->type = REWIND_TYPE_CLOSE;
  sendMsg(rd, sizeof(struct RewindData));

  m_state = DISCONNECTED;
  m_ping_timer.setEnable(false);
} /* RewindLogic::sendCloseMessage */


void RewindLogic::sendConfiguration(void)
{

} /* RewindLogic::sendConfiguration */


void RewindLogic::mkSHA256(std::string pass, int len, uint8_t hash[])
{
  //uint8_t t_hash[];
  SHA256_CTX context;
  sha256_init(&context);

  size_t pl = pass.length();
  unsigned char pbuf[pl];
  copy(pass.begin(), pass.end(), pbuf);
  sha256_update(&context, pbuf, pl);
  sha256_final(&context, hash);

} /* RewindLogic:mkSha256*/


void RewindLogic::handleDataMessage(std::string dmessage)
{

} /* RewindLogic::handleDataMessage */

/*
 * This file has not been truncated
 */
