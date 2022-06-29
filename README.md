# highfleet-aimer
A program using computer vision to predict aiming in the game HighFleet. It's tuned for 180mm guns but it should more or less work for 130mm, 100mm and 57mm cannons.

(The tracking is very dodgy, please help me do OpenCV stuff better)

This only runs fast enough if you compile in Release mode (Debug is too slow to be useful)

Be sure to run HighFleet in windowed mode (1920x1080). Hold left ctrl to auto aim.

I've only tested this on my 1440p monitor (at 125% DPI), no clue if this works in other resolutions/DPI settings.

I forgot all my computer science skills help

This requres OpenCV (I used 4.6.0 but maybe you can get away with older stuff), and I built it with CMake. Honestly OpenCV + CMake + VS Code + Windows was a pain to setup, you can try guides like https://www.youtube.com/watch?v=6nLfS6GWbXw or https://www.youtube.com/watch?v=m9HBM1m_EMU for help
