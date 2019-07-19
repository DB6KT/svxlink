###############################################################################
#
# SipLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[SipLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval SipLogic {

#
# Checking to see if this is the correct logic core
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
}


#
# Executed when the SvxLink software is started
#
proc startup {} {
  global logic_name;
  puts "starting $logic_name";
}


#
# Executed when an unknown DTMF command is entered
#
proc unknown_command {cmd} {
  playMsg "Core" "unknown_command";
  playNumber $cmd;
}


#
# Executed when an outgoing call is not permitted (dropped)
#
proc drop_outgoing_call {caller} {
  puts "Dropping outgoing call to $caller due to configuration.";
  playMsg "Sip" "call_not_allowed";
  playSilence 200;
}


#
# Executed when an incoming call is rejected
#
proc reject_incoming_call {caller} {
  puts "Rejecting incoming call from $caller due to configuration.";
  playMsg "Sip" "reject_incoming_call";
  playSilence 200;
}


#
# Executed when an entered DTMF command failed
#
proc command_failed {cmd} {
  playMsg "Core" "command_failed";
  playNumber "Core" $cmd;
}


#
# Executed when a link to another logic core is activated.
#   name  - The name of the link
#
proc activating_link {name} {
  playMsg "Core" "activating_link";
  playMsg "Core" $name;
}


#
# Executed when a link to another logic core is deactivated.
#   name  - The name of the link
#
proc deactivating_link {name} {
  playMsg "Core" "deactivating_link";
  playMsg "Core" $name;
}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#   name  - The name of the link
#
proc link_not_active {name} {
#  Logic::link_not_active $name;
}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#   name  - The name of the link
#
proc link_already_active {name} {
#  Logic::link_already_active $name;
}


#
# Executed once every whole minute
#
proc every_minute {} {
  Logic::every_minute;
}


#
# Executed once every second
#
proc every_second {} {
  Logic::every_second;
}


#
# Executed once every whole minute to check if it's time to identify
#
proc checkPeriodicIdentify {} {
# Logic::checkPeriodicIdentify;
}


#
# Executed if an incoming call is pickuped
#
proc pickup_call {caller} {
  puts "pickup call from: $caller";
  playSilence 500;
  playMsg "Sip" "announce_party";
  playSilence 100;
  spellNumber [getCallerNumber $caller];
  playSilence 500;
}


#
# Executed if an outgoing call is pending
#
proc calling {caller} {
  playSilence 500;
  playMsg "Sip" "calling";
  playSilence 100;
  spellNumber [getCallerNumber $caller];
}


#
# Executed if an outgoing call is pending
#
proc outgoing_call {caller} {
  puts "outgoing_call $caller";
}


#
# Executed if somebody is ringing
#
proc ringing {caller} {
  puts "$caller ringing";
  playMsg "Sip" "ringtone";
}


#
# Executed if an incoming call is established
#
proc incoming_call {caller} {
  playSilence 500;
  playMsg "Sip" "incoming_phonecall";
  playSilence 100;
  playNumber [getCallerNumber $caller];
  playSilence 200;
}


#
# Executed when a DTMF digit has been received
#   digit - The detected DTMF digit
#   code  - The duration, in milliseconds, of the digit
#
# Return 1 to hide the digit from further processing in SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_digit_received {digit caller} {
  puts "Dtmf digit received $digit from $caller";
}


#
# Executed when a DTMF command has been received
#   cmd - The command
#
# Return 1 to hide the command from further processing is SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_cmd_received {cmd} {
  return 0;
}


#
# Executed when the node is being brought online after being offline
#
proc logic_online {online} {
  puts "$online";
}


#
# Executed when the party isn't at home
#
proc call_timeout {} {
  puts "Called party is not at home";
  playMsg "Sip" "person_not_available";
  playSilence 200;
}


#
# Executed when a call has hangup
#
proc hangup_call {uri duration} {
  puts "Hangup call $uri ($duration seconds)";
  playMsg "Sip" "hangup_call";
  playMsg 200;
}


#
# Registration state (code)
#
proc registration_state {server state code} {
  variable status;
  if {$state} {
    set status "registered";
  } else {
    set status "unregistered";
  }
  puts "Registration on $server, code $code ($status)";
}


#
# Returns the phone number of the remote station
# URI: <sip:12345678@sipserver.com:5060>
#        -> 12345678
#
proc getCallerNumber {uri} {
  set r [split $uri @];
  set number [split [lindex $r 0] :];
  return [lindex $number 1];
}

# end of namespace
}


#
# This file has not been truncated
#
