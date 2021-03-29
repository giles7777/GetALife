# Node_v13

## Light Capabilities

Compact message format, with all options transmitted:

>  {"light":{"bright":255,"interval":100,"increment":1,"blend":1,"palette":[[0,0,255],[0,0,139],[0,0,139],[0,0,139],[0,0,139],[0,0,139],[0,0,139],[0,0,139],[0,0,255],[0,0,139],[135,206,235],[135,206,235],[173,216,230],[255,255,255],[173,216,230],[135,206,235]]}}

Only portions of the message need to be transmitted, and other prior settings will be retained:

> {"light":{"bright":255}}

**NOTE** that palette will fill [0,0,0] (black) for missing array pieces.  

Expanded definitions:

{
- "light": {
    - "bright": 0-255;overall brightness of the LED array; *void Light::setBrightness(uint8_t bright)*
    - "interval": 0-9999 ms; interval between color change
    - "increment": 0-255; increment in palette space
    - "blend": 0=no blending/1=linear blending; blending between palette colors
    - "palette": [
        - [ 0-255; red level in palette entry 0, 0-255; green level in palette entry 0, 0-255; blue level in palette entry 0 ],
        - *(repeats 15 times; 16 total palette entries)*
    - ]
	- "sparkle": 0-9999; every *interval*, we have a 1/*sparkle* probability of sparkling.
-  }

}

If we access the message as a JsonDocument:

    JsonDocument aPal = message["light"]["animPal"];

This message drives the following animation sequence on the LED's:

    EVERY_N_MILLISECONDS( aPal["interval"] ) {
	   // what was done last round
       static uint8_t index = 0;
	   static CRGB lastColor = CRGB::Black;

	   // what is done this round
       index += aPal["increment"]; 
	   CRGB newColor = ColorFromPalette( aPal["palette"], index, aPal["bright"], aPal["blend"] );
	   
       // adjust the array
	   for(byte i=0; i<N_LED; i++) leds[i] += (newColor - lastColor);
	   
	   // track our changes
	   lastColor = newColor;

       // show       
       FastLED.show();
    }



## Sound Capabilities

