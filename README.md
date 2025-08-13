# Fractus
Real-time fractal-generating software


## How Does it Work:
The program uses C++ with OpenGL rendering. The user is able to create sub-screens that have the same aspect ratio of the user's display. The sub-screens are able to be moved, scaled, and rotated by the user. They have a very low opacity and can be stacked on top of each other. Every frame, the entire screen is captured and pasted on top of each of the sub-screens. Because of this, self-similar fractals can be generated with ease. 


## Controls:
| Control | Function |
| :------- | :------: |
| Middle Click | Create sub-screen |
| Left Click | Select sub-screen + Move sub-screen |
| Right Click | Delete sub-screen |
| Scroll | Scale selected sub-screen |
| A/D | Rotate selected sub-screen |


## Optimizations
This project was originally made in Python. The major problem was that it was heavily dropping in frame rate whenever hundreds of alpha values were being added (sometimes as bad as 15 fps). Since I loved the idea of this, I badly wanted to improve the effieciency, but I couldn't quite get to the level I wanted in Python. Knowing this, I decided to make a switch to C++ in hopes of great performance. Discovering that C++ can have high and low-level integrations, I decided to opt for somewhere in the middle because I think a lower-level integration could have diminishing returns. I used OpenGL because I knew rendering was the real bottleneck in this project.


## Difficulties:
When I made the switch to C++, I was expecting it to be a smooth process since the actual logic and math is already done. I was wrong. The real problem was that I was 5 weeks away from my first C++ class. I had no idea what I was doing; it wasn't really fun to figure out what was going on while trying to build a program of this calibre. There were times where I was just hoping to build without errors. In hindsight, I probably should have just waited for my class to start. I made so much more progress after I knew what was happening. Another huge challenge was OpenGL's blending system. It really puts all the power in the programmer's hands at the cost of it being finicky and hard to understand. For example, it was surprisingly hard to make the program handle all colors correctly; because of additive blending, adding the same color hundreds of times approaches white instead of the intended color.


## Working on Implementing:
I am trying to statically link my program so that it can be run on all computers without an install, but this seems to be challenging.

## Examples:

**Screenshot #1: Fuzz**
<img width="2560" height="1600" alt="Screenshot (32)" src="https://github.com/user-attachments/assets/15ed9b21-8ca4-47af-93fb-ee34e59324a1" />

**Sreenshot #2: Dragon**
<img width="2560" height="1600" alt="Screenshot (34)" src="https://github.com/user-attachments/assets/b1e29343-8629-469a-b457-089d56230ad0" />

**Screenshot #3: Chains**
<img width="2560" height="1600" alt="Screenshot (31)" src="https://github.com/user-attachments/assets/f483a0c8-112d-44f5-85cc-c92e89b42e8f" />

**Sreenshot #4: Sierpinski's Pyramid**
<img width="2560" height="1600" alt="Screenshot (33)" src="https://github.com/user-attachments/assets/11e0f85e-6199-4e82-9df4-2410a1ee6b9f" />
