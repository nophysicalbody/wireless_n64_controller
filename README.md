# wireless_n64_controller

At this point, this is just an experiment.

Sender application (arduino UNO)

* Reads N64 controller state
* Sends this state over RFM69

Receiver application (arduino DUE)

* Receives n64 data from RFM69
* Displays values on serial console