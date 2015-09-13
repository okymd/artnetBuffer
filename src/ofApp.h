#pragma once

#include "ofMain.h"
#include "ofxUdpManager.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void exit();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);



	public:
		char * buffer; //
		ofxUDPManager udpSender[5];
		ofxUDPManager udpReceiver;

		static const int MAX_NUM_UNIVERSES = 10;
		static const unsigned int MAX_FRAME_NUM = MAX_NUM_UNIVERSES*256;
		static const unsigned int ARTNET_PACKET_SIZE = 530;// Art-net packet size
		static const unsigned int NUM_LEDS = 192;
		static const unsigned int PACKET_SIZE = NUM_LEDS * 3;// for Arduino packet size
				
		unsigned int numUniverses;
		unsigned int maxNumUniverses;
		 

		enum {
			REC,
			PLAY,
			THROUGH
		};
		int mode;



		vector<char*> frames;
		
		void storeFrame(const char* buffer,int frame);
		void recallFrame(int frameNum);

		void allocateBuffer();
		void releaseBuffer();

		void sendPacket(char* artnetPacket);
		void receivePacket(char* artnetPacket);
		void parseArtnetDMX(char* artnetPacket);
		void sendTestPacket(int universe,ofColor color);
		int  getUniverse(const char* artnetPacket);
		
		//
		bool bReceive[MAX_NUM_UNIVERSES];
		bool bSend[MAX_NUM_UNIVERSES];


	//ofParamer
		ofParameter<int> lastFrame;
		ofParameter<string> status;
		ofxIntSlider currentFrame;
		ofxIntSlider maxFrame;
		ofxIntSlider fps;
		
	//GUI
		ofxPanel gui;
		ofxToggle bRecording;
		ofxToggle bPlaying;
		ofxButton btnTriple;
		ofxButton btnDouble;
		ofxButton btnNormal;
		ofxButton btnTest;
		
		ofxLabel  labelStatus;
		

	//GUI handler
		void onRec(bool &bRec);
		void onPlay(bool &bPlay);
		void onChangeFPS(int &val);
		void onTriple(){ fps=75;ofSetFrameRate(fps);};
		void onDouble(){ fps=50;ofSetFrameRate(fps);};
		void onNormal(){ fps=25;ofSetFrameRate(fps);};
		void onTest();


		
};
