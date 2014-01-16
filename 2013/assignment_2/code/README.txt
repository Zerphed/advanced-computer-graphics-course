Name: Joonas Nissinen
Student id: ******
Assignment: Assignment 2

1. COLLABORATION:
-----------------

I had some collaboration with Paula Jukarainen, mainly about the SAH implementation and sampling.


2. REFERENCES:
--------------

There were a lot of good sources, and I can't hope to list all the materials that I used. The lecture slides and the links they contained provided a lot of help.

* Realtime Rendering 2nd Edition
* Realistic Ray Tracing 2nd Edition
* Computer Graphics with OpenGL 4th Edition
* http://www.flipcode.com/archives/Raytracing_Topics_Techniques-Part_7_Kd-Trees_and_More_Speed.shtml


3. KNOWN PROBLEMS:
------------------

There are no known problems with the code as far as I know. There is some overlapping implementation with bounding boxes and sampling since I didn't have the time to incorporate the refactored classes into the rest of the code.

One weird issue is that the program loads a 480 triangle version of the cornell scene by default from preset 1. You can load the tesselated 60k triangle version from scenes/cornell_course/cornell-tesselated.


4. EXTRA CREDIT:
----------------

4.1. Multithreading: I implemented multithreading for the radiosity calculation. This wasn't hard since most of my code was thread safe as it was.

4.2. Regular/Uniform Sampling: I implemented uniform/regular (grid) sampling for the direct as well as the indirect lighting. This was pretty straightforward and I further improved my Sampler/Sequence classes but didn't have time to incorporate the refactored code into my solution. 

4.3. Stratified Sampling: I implemented stratified sampling for the direct as well as the indirect lighting. This was very straightforward after implementing the regular/uniform sampling.

4.4. Quasi Monte Carlo Sampling: I implemented Halton sequence sampling for the direct as well as the indirect lighting, I also precalculated the base 2 and base 3 halton values which improved the computing speed a little. 

4.5. SAH: Out of interest I decided to implement SAH to improve the calculation speed in my scenes. SAH implementation took quite a bit amount of work before realizing how to get it right, but for the scenes where its realistic to use SAH it was worth it. I improved the SAH calculation time by precomputing triangle centroids and bounding boxes for each triangle, but it's still painstakingly slow. 

You can switch between the sampling modes and the BVH construction modes from the GUI. If you decide to switch the BVH construction mode you need to reload the mesh.

I also implemented timers for the BVH building and radiosity computation to compare performances with different settings. The information is printed to the console.

Furthermore I tried to refactor bits and pieces of the code in order to improve the overall performance, but the refactoring project is still incomplete.


5. ARTIFACTS:
-------------

The required artifacts can be found in the artifacts folder.


6. COMMENTS:
------------

I enjoyed doing this exercise a lot even though every now and then debugging was awful. I would also like to point out that debugging would be easier if the model solution functioned correctly. Next time providing better scenes for e.g. texture tests would be nice.

I hope the scene images I provided are sufficient. As I understood you wanted the scenes 1 and 2 with their default settings (pressing keys 1 and 2).

Thank you for allowing me this extended deadline. I am currently swamped with work but I try to make the best with what I have.

