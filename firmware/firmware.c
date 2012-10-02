#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "Descriptors.h"
#include "Config/AppConfig.h"

#include <LUFA/Drivers/USB/USB.h>

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = 1,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint           =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};

void SetupHardware(void);

int main(void)
{
	sei();
	SetupHardware();

	for (;;)
	{
		MIDI_EventPacket_t ReceivedMIDIEvent;
		if (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent))
		{
			if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_OFF)) && (ReceivedMIDIEvent.Data1 == 0x80))
			{
				if (ReceivedMIDIEvent.Data2 == 60) {
					TCCR1A &= ~_BV(COM1A1);
				} else if (ReceivedMIDIEvent.Data2 == 62) {
					TCCR1A &= ~_BV(COM1B1);
				} else if (ReceivedMIDIEvent.Data2 == 64) {
					TCCR1A &= ~_BV(COM1C1);
				} else {
					if (ReceivedMIDIEvent.Data2 > 64 && ReceivedMIDIEvent.Data2 < 72) {
						char leds = ReceivedMIDIEvent.Data2 - 64;
						if (leds & 1)
							TCCR1A &= ~_BV(COM1A1);
						if (leds & 2)
							TCCR1A &= ~_BV(COM1B1);
						if (leds & 4)
							TCCR1A &= ~_BV(COM1C1);
					}
				}
			}
			else if (((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && (ReceivedMIDIEvent.Data1 == 0x90)) ||
					 ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, 0xa0)) && (ReceivedMIDIEvent.Data1 == 0xa0)))
			{
				if (ReceivedMIDIEvent.Data2 == 60) {
					TCCR1A |= _BV(COM1A1);
					OCR1A = ReceivedMIDIEvent.Data3 << 1;
				} else if (ReceivedMIDIEvent.Data2 == 62) {
					TCCR1A |= _BV(COM1B1);
					OCR1B = ReceivedMIDIEvent.Data3 << 1;
				} else if (ReceivedMIDIEvent.Data2 == 64) {
					TCCR1A |= _BV(COM1C1);
					OCR1C = ReceivedMIDIEvent.Data3 << 1;
				} else {
					if (ReceivedMIDIEvent.Data2 > 64 && ReceivedMIDIEvent.Data2 < 72) {
						char leds = ReceivedMIDIEvent.Data2 - 64;
						if (leds & 1) {
							TCCR1A |= _BV(COM1A1);
							OCR1A = ReceivedMIDIEvent.Data3 << 1;
						}							
						if (leds & 2) {
							TCCR1A |= _BV(COM1B1);
							OCR1B = ReceivedMIDIEvent.Data3 << 1;
						}							
						if (leds & 4) {
							TCCR1A |= _BV(COM1C1);
							OCR1C = ReceivedMIDIEvent.Data3 << 1;
						}							
					}
				}
			}
			else if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, 0xb0)) && (ReceivedMIDIEvent.Data1 == 0x80)) {
				// PWM change (Control)
				if (ReceivedMIDIEvent.Data2 == 20) {
					OCR1A = ReceivedMIDIEvent.Data3 << 1;
				} else if (ReceivedMIDIEvent.Data2 == 21) {
					OCR1B = ReceivedMIDIEvent.Data3 << 1;
				} else if (ReceivedMIDIEvent.Data2 == 22) {
					OCR1C = ReceivedMIDIEvent.Data3 << 1;
				} else {
					if (ReceivedMIDIEvent.Data2 > 22 && ReceivedMIDIEvent.Data2 < 30) {
						char leds = ReceivedMIDIEvent.Data2 - 64;
						if (leds & 1)
							OCR1A = ReceivedMIDIEvent.Data3 << 1;
						if (leds & 2)
							OCR1B = ReceivedMIDIEvent.Data3 << 1;
						if (leds & 4)
							OCR1C = ReceivedMIDIEvent.Data3 << 1;
					}					
				}
			}			
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}


/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{	
	PORTB = 0;
	PORTC = 0;
	DDRB |= _BV(7);
	DDRC |= _BV(6) | _BV(5);
	
	// Set up PWM
	OCR1A = 0xff;
	OCR1B = 0xff;
	OCR1C = 0xff;
	TCCR1B = _BV(WGM12) | _BV(CS10); // Fast PWM, no prescaling
	TCCR1A = _BV(WGM10);
	
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	//TCCR1A |= _BV(COM1A1);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);

	//TCCR1A |= _BV(COM1B1);
	//if (ConfigSuccess)
	//	TCCR1A |= _BV(COM1C1);
	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}
