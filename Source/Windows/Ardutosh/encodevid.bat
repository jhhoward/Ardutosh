\Tools\ffmpeg\bin\ffmpeg.exe -r 50 -f image2 -framerate 50 -i Frame%05d.png -vf scale=256:128 -sws_flags neighbor out.gif 
