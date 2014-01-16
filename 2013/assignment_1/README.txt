Name: Joonas Nissinen
Student number: ******
Assignment: Assignment 1

1. COLLABORATION:
-----------------

I had some minor collaboration with Paula Jukarainen, but it was strictly about exchanging ideas and debugging the code every now and then.


2. REFERENCES:
--------------

Of course there were plenty of references and random googling but mostly I sufficed with the help of Wikipedia and the lecture slides. Below is a list of the most important references used:

http://en.cppreference.com/w/
http://en.wikipedia.org/wiki/Halton_sequence
http://pathtracing.wordpress.com/2011/03/03/cosine-weighted-hemisphere/

Computer Graphics with OpenGL 4th Edition by Hearn, Baker and Carithers


3. KNOWN PROBLEMS:
------------------

There are no known problems with the code, at least as far as the basic requirements go. However I removed the precentage counter from the terminal because it didn't go right with the threading I implemented.


4. EXTRA CREDIT:
----------------

I implemented multithreading to improve the program performance. The implementation uses the MulticoreLauncher provided by the course to divide the picture into scanlines which are then rendered in individual threads. Most of the work is done by the framework, but it got some time to refactor the code for multithreading and find the right way to use the MulticoreLauncher.

I also implemented Halton low discrepancy sampling for ambient occlusion. It took some time to realize how to get the right samples from the sequence, but it didn't take more than a few hours to get it working.


5. COMMENTS:
------------

Well I liked the exercise but felt like it wasn't as well specified as the exercises in the fall's class. I also sort of disliked the amount of C mixed in with my C++, but I guess that's to be expected in graphics programming :).