This project combines the use of OpenAI with OpenWeatherMap.  There can be a considerable time gap between the
curent weather report and the forecast weather report when using the OpenWeahterMap free API.  this project 
"fills in the gaps" by consulting OpenAI.  So far, It seems to do a pretty good job. 
This project uses the Cheap Yellow Display (CYD).  You will need a free OpenWeatherMap key and an OpenAI key.
Here is a link to the 3D printed case. https://www.thingiverse.com/thing:6953794

![IMG_1577](https://github.com/user-attachments/assets/b96f4b74-9d93-4a9d-a341-9bf10d177ebd)

As always, there is room for improvement. That's why test is in the name. Here are some possibe changes:
    
    Make text banner in the display move from top to bottom depending on where the graph is (easy).
    
    Strip off the forecast data that is a bit too far in the future before uploading to the AI.
    This would decrease to cost of the AI API access. (hard)

    Note the function that displays the Title, File Name, Compile Time.
    You can modify this too your liking. (easy).

    Pull the OpenAI prompt text out of the sketch and place in a github data file.  It would be downloaded 
    as needed (See SprinklerInhibit project).  This would allow the propt to be refined without recompiling. (moderate)

    Notes:  
    
    You must set up your TFT_eSPI library for the cheap Yellow dosplay.  There are lots of 
    YouTube videos on this.

    As noted in the banner, you will need an OpenAI API key.  It costs money but it is cheap.  You will also need
    the OpenWeatherMap free API key.  Both of these are pretty easy to get.

    Kudos to the folks that worte the Libraries used in this sketch.  I stand in awe.
    
