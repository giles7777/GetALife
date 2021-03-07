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
    - "bright": 0-255; overall brightness of the LED array
    - "interval": 0-9999 ms; interval between color change
    - "increment": 0-255; increment in palette space
    - "blend": 1-5; blending between palette color; see FastLED::blend
    - "palette": [
        - [ 0-255; red level in palette entry 0, 0-255; green level in palette entry 0, 0-255; blue level in palette entry 0 ],
        - *(repeats 15 times; 16 total palette entries)*
    - ]
-  }

}

These values drive the following animation sequence on the LED's:

>  EVERY_N_MILLISECONDS( *"interval"* ) {
>    static uint8_t startIndex = 0;
>    startIndex += *"increment"*; 
>
>    CRGB color = ColorFromPalette( *"palette"*, startIndex, *"bright"*, *"blend"* );
>    fill_solid(leds, NUM_LEDS, color);
>
>    FastLED.show();
>  }



## Sound Capabilities

