# **USC CSCI 420: Computer Graphics**  
#### \- taught by Dr. Jernej Barbič  

## **Programming Assignment 2: Simulating a Roller Coaster**  

    Operating System: macOS 10.15
    Source Code Editor: Sublime Text, Version 3.2.2, Build 3211
    Programming Language: C++
    API: OpenGL (Core Profile)

### **ASSIGNMENT DETAILS:**
In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.  

- Look into the file ```Assignment-2-details.pdf```  
                OR
- Go to [![http://barbic.usc.edu/cs420-s20/assign2/index.html](http://barbic.usc.edu/cs420-s20/assign2/index.html)](http://barbic.usc.edu/cs420-s20/assign2/index.html)

### **HOW TO EXECUTE THE CODE (For macOS):**
1. Go to folder ```HW2```
2. Compile using the command: ```make```.  
Note: To delete all the object files and executables:```make clean```
3. To run type:  
    ```
    ./<your-exe-filename> <track-filename>
    ```  
    Sample:  
    ```
    ./hw2 track.txt
    ```  
  
track.txt contains,  
    - the count of the number of splines to be used for the path of roller coaster  
    and  
    - the names of the SP files containing the splines.  
e.g.

    2
    splines/myRollerCoaster1.sp
    splines/myRollerCoaster2.sp

### **KEYBOARD/MOUSE CONTROLS:**
#### **Keyboard:**
- Toggling ```r``` turns continuous rotation of the scene(centered at the world origin) about the y axis, on and off. When toggled off, the camera returns to the coaster-ride starting from where it had last left.
- ```x``` takes a single screenshot of the current state of the window running the application.
- ```s``` takes 300 screenshots
- ```ESC``` exits the application.

#### **Mouse:**
While ```r``` is toggled on,

- ```Dragging mouse with the left mouse button clicked``` produces further rotation about the x and y axes.
- ```Dragging mouse with the middle mouse button clicked``` produces rotation about the z axis.
- ```Holding 't' and dragging mouse with the left mouse button clicked``` produces translation along the x and y axes.
- ```Holding 't' and dragging mouse with the middle mouse button clicked``` produces translation along the z axis.

The mouse controls are disabled for when camera is riding the roller-coaster as that would disrupt the ride view which is constantly changing.

### **ADDITIONAL FEATURES(EXTRA CREDIT CONSIDERATIONS):**

1. **T-shaped rail cross section**  
    The T shape indicates that the bar (-) is the upper part (the side the camera rides on) of the rail and the stick (|) is the lower part.

2. **Double Rail**  
    Rendered double rail along with the crossbars(called sleepers) in between them at intervals.

3. **STANFORD DRAGON**  
    Created a ```.obj``` file loader (objloader.h), and inserted the STANFORD DRAGON (.obj file taken from Prof. Barbič's VegaFEM-v4.0 object repository) into the scene. After many attempts placed it suitably beside the track in such a way that the support bars don't go through the dragon and it also makes the ride view interesting.
    
4. **Support for the tracks**  
    Support for the tracks from the ground to make it more realistic.

5. **Sky-box**  
    Used very high resolution (2048x2048) photos which were created by ```Emil Persson, aka Humus```.
    Made sure that the texture images have (Width * #colorChannels) as a multiple of 4.

6. **Self-created tracks**  
    Created two sequences of splines (```myRollerCoaster1.sp```, ```myRollerCoaster2.sp```). Designed them to go about the STANFORD DRAGON to make the scene more appealing. The other sequences of splines (the ones given with starter code) do inversions that would require a different type of support system. The code can handle those as well, but the support system will look ugly for them. The two sequences of splines also complement each other by one starting in a scene near where the other ends. The sequences of splines also are designed to cross over/below with itself at places. The sequences of splines have also been used to mimic modification of camera velocity (as described below in point 8).

7. **Track from several sequences of splines**  
    The code can handle any number of sequences of splines, and keeps looping over all of them till the code is  exited.
    To demonstrate this fact, I created two sequence of splines (instead of one).

8. **Render environment in a better manner**
    Used separate pipeline program to shade the STANFORD DRAGON with a different coloration and material look. 

9. **Velocity modification with which the camera moves**  
    (BUT NOT USING THE EQUATION FOR PHYSICALLY REALISTIC MOVEMENT IN TERMS OF GRAVITY)  
    I realized while making the sequences of splines that, by varying the distance between the control points in the file, we can control the speed of the ride. In this code, u runs between [0,1] \(both inclusive\), in steps of 0.01. Therefore, there are 101 spline points per spline segment here. Since the u parameterization is fixed, a short spline segment means, there will be 101 spline points in that short distance, making the camera move very slowly on it (as the camera on this code derives its speed from the number of spline points it skips per frame - in this code it is 2). And a long spline segment will have the 101 spline points more spaced apart relatively, making the camera move faster on it. Keeping this in mind, I designed the sequences of splines with longer segments towards the lower ends of paths going down and just coming up; while having short segments towards the top.  
I realize however that the velocity variation this way becomes dependent on the sequences of splines used instead of the more generic alternative of designing movement based on gravity.

10. **Derivation of the steps that lead to the physically realistic equation of updating u**  
    view the file ```HW2/PROOF OF u UPDATE EQUATION - SHREYA BHAUMIK.pdf```.

11. **Rotate and view scene from center**  
    Since the scene was visible only from the roller coaster ride, the overall structure and feel of the scene was not felt. Therefore, I added a feature - on pressing ```r``` on the keyboard, the camera goes to the center of the skybox and the environment rotates about the y axis. During this rotation, one can also rotate, translate further using mouse controls (please see Keyboard/Mouse controls section below). The ```r``` is a toggle key which one needs to press again to get back to the camera riding on the track. One can keep toggling between coster-ride and rotation. Every time the coaster-ride starts from where it had left before the rotation.

### **OPEN-ENDED PROBLEMS:**
1. Computed 2 points based on per spline point position for the corresponding left and right rail points. Created rail T upper part (-) cross-section vertices using the normals and binormals. Used the vertices made for the left and right rails to put crossbars in-between the rails in certain intervals.
2. Made lower part of T cross-section (|) with points made from further deriving from the 2 rail points mentioned above, correspondingly for the left and right rail support system. Computed the vertices using the normals and binormals.
3. Created support system cross-section vertices using the rail T upper part (-) cross-section lower face vertices and the corresponding vertices on the ground.

These are 3 of the notable open-ended problems tackled in the homework.

### **NECESSARY FILES AND FOLDERS LOCATIONS:**
- The ```.sp``` spline files are in the folder "HW2/splines".
- The images used for skybox are in the folder "HW2/skybox".
- The ```.obj``` file used for STANFORD DRAGON is in the folder "HW2/objects".

### **EXHIBIT:**
![RollerCoaster](sample-images/RollerCoaster.png)  
![DragonInScene](sample-images/DragonInScene.png)  
![Scene](sample-images/Scene.png)  
[![https://www.youtube.com/watch?v=lIiI4SpHbTc](https://www.youtube.com/watch?v=lIiI4SpHbTc)](https://www.youtube.com/watch?v=lIiI4SpHbTc)

### **LICENSE FOR IMAGES(SKYBOX) AND OBJ(DRAGON):**
#### **Skybox images:**
The images used are photos which were created by ```Emil Persson, aka Humus```. The photos have the following license agreement:

    Author
    ======

    This is the work of Emil Persson, aka Humus.
    http://www.humus.name


    License
    =======

    This work is licensed under a Creative Commons Attribution 3.0 Unported License.
    http://creativecommons.org/licenses/by/3.0/

#### **Dragon object:**
The ```.obj``` file is taken from Prof. Barbič's VegaFEM-v4.0 object repository. The ```.obj``` file has the following license agreement:  

    The volumetric mesh and its surface mesh were created by processing the dragon mesh available at:

    Stanford University Computer Graphics Laboratory
    http://graphics.stanford.edu/data/3Dscanrep/

    The Stanford website states the following usage:

    -----
    Please be sure to acknowledge the source of the data and models you take from this repository.
    In each of the listings below, we have cited the source of the range data and reconstructed models.
    You are welcome to use the data and models for research purposes.
    You are also welcome to mirror or redistribute them for free.
    Finally, you may publish images made using these models, or the images on this web site,
    in a scholarly article or book - as long as credit is given to the Stanford Computer Graphics Laboratory.
    However, such models or images are not to be used for commercial purposes,
    nor should they appear in a product for sale (with the exception of scholarly journals or books), without our permission.
    -----

    We release our derived dragon meshes under the same terms as the terms of the Stanford University Computer Graphics Laboratory.

