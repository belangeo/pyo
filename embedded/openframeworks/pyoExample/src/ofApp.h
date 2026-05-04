#ifndef __of_app_h
#define __of_app_h

#include "ofMain.h"
#include "PyoClass.h"
#include <string>
#include <vector>

#define CURSORBLINK 500

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		void audioIn(ofSoundBuffer & buffer);
		void audioOut(ofSoundBuffer & buffer);

		void exit();

		ofTrueTypeFont font;

		ofSoundStream soundStream;

		Pyo pyo;

		float interpreterRectHeight;
		float interpreterRectWidth;
		float interpreterRectXPos;
		float interpreterRectYPos;

		unsigned long cursorTimeStamp;
		bool showCursor;
		float cursorXPos;
		float cursorYPos;
		float cursorHeight;

		bool exiting;

		float letterWidth;
		string typeStr;
		string stdoutStr;
		string stderrStr;
};

#endif
