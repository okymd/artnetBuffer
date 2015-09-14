#pragma once

#include "ofMain.h"
#include "ofxUdpManager.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"


#define ART_NET_ID "Art-Net\0"
#define ART_DMX 0x5000
#define ART_DMX_START 18


static const int ARTNET_PORT = 6454; 
static const unsigned int ARTNET_PACKET_SIZE = 530;// Art-net packet size

//LED Devices
static const unsigned int NUM_LEDS = 192;
static const unsigned int PACKET_SIZE = NUM_LEDS * 3;// for Arduino packet size
static const int NUM_REMOTE_DEVICES = 5;


//
static const int MAX_NUM_UNIVERSES = NUM_REMOTE_DEVICES*2;
		

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
		char * frameBuffer; //
		ofxUDPManager udpSender[5];
		ofxUDPManager udpReceiver;

		
				
		unsigned int numUniverses;
		unsigned int maxNumUniverses;
		 

		enum {
			REC,
			PLAY,
			THROUGH
		};
		int mode;

		static const unsigned int MAX_FRAME_NUM = 1024;
		static const unsigned int FRAME_SIZE = ARTNET_PACKET_SIZE*MAX_NUM_UNIVERSES;

		vector<char*> frames;
		
		void storePacket(const char* artnetPacket);
		void recallFrame(int frameNum);
		void storeFrame(char* frameBuffer,int frameNum);
		
		void sendPacket(int index,char* artnetPacket);
		void sendFrame(char* frameBuffer);


		void allocateFrameBuffer();
		void releaseFrameBuffer();

		void receivePacket(char* artnetPacket);
		void parseArtnetDMX(char* artnetPacket);
		void sendTestPacket(int index,ofColor color);
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
		ofxToggle bThrough;
		ofxButton btnTriple;
		ofxButton btnDouble;
		ofxButton btnNormal;
		ofxButton btnTest;
		ofxButton btnReconnect;
		
		ofxLabel  labelStatus;
		

	//GUI handler
		void onRec(bool &bRec);
		void onPlay(bool &bPlay);
		void onChangeFPS(int &val);
		void onTriple(){ fps=75;ofSetFrameRate(fps);};
		void onDouble(){ fps=50;ofSetFrameRate(fps);};
		void onNormal(){ fps=25;ofSetFrameRate(fps);};
		void onTest();
		void onReconnect();


		
};
