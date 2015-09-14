#include "ofApp.h"


////Arduino IP's
const string remoteIP[NUM_REMOTE_DEVICES] ={
	"192.168.1.200",
	"192.168.1.201",
	"192.168.1.202",
	"192.168.1.203",
	"192.168.1.204"
};

////Arduino IP's
//const string remoteIP[5] ={
//	"192.168.0.200",
//	"192.168.0.201",
//	"192.168.0.202",
//	"192.168.0.203",
//	"192.168.0.204"
//};


//--------------------------------------------------------------
void ofApp::setup(){

	//Receiver Setup
	udpReceiver.Create();
	if(udpReceiver.Bind(ARTNET_PORT)<0){
		printf("UDP Error:can't bind\n");
	};
	udpReceiver.SetNonBlocking(true);

	printf("UDP Receive Buffer Size:%d\n",udpReceiver.GetReceiveBufferSize());

	//Sender Setup
	for(int i=0;i<NUM_REMOTE_DEVICES;i<i++){
		udpSender[i].Create();
		if(udpSender[i].Connect(remoteIP[i].c_str(),ARTNET_PORT)){;
			udpSender[i].SetNonBlocking(true);	
			printf("UDP Sender[%d] Connected to:%s\n",i,remoteIP[i].c_str());
		}else{
			printf("UDP Sender[%d] can't connect to:%s\n",i,remoteIP[i].c_str());
		}
	}

	//Osc Sender Setup
	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		oscSender[i].setup(remoteIP[i].c_str(),OSC_PORT);
	}

	frameBuffer = new char[ARTNET_PACKET_SIZE*MAX_NUM_UNIVERSES];

	allocateFrameBuffer();

	mode = THROUGH;
	maxNumUniverses=0;

	//gui setup
	btnTriple.addListener(this,&ofApp::onTriple);
	btnDouble.addListener(this,&ofApp::onDouble);
	btnNormal.addListener(this,&ofApp::onNormal);
	fps.addListener(this,&ofApp::onChangeFPS);
	bright.addListener(this,&ofApp::onChangeBright);

	bRecording.addListener(this,&ofApp::onRec);
	bPlaying.addListener(this,&ofApp::onPlay);
	btnTest.addListener(this,&ofApp::onTest);
	btnReconnect.addListener(this,&ofApp::onReconnect);

	ofxGuiSetTextPadding(4);
	ofxGuiSetDefaultWidth(ofGetWindowWidth());

	gui.setup();
	gui.setPosition(0,60);
	gui.add(labelStatus.setup(status));
	gui.add(bRecording.setup("Rec",false));
	gui.add(bPlaying.setup("Play",false));
	gui.add(bThrough.setup("Through",true));
	gui.add(bPause.setup("Pause",false));
	gui.add(currentFrame.setup("Frame",0,0,MAX_FRAME_NUM));
	gui.add(maxFrame.setup("Max Frame",MAX_FRAME_NUM,1,MAX_FRAME_NUM));
	gui.add(bright.setup("Bright",255,0,255));
	gui.add(btnTriple.setup("FPSx3"));
	gui.add(btnDouble.setup("FPSx2"));
	gui.add(btnNormal.setup("FPSx1"));
	gui.add(fps.setup("FPS",75,25,100));
	gui.add(btnTest.setup("TEST"));
	gui.add(btnReconnect.setup("Reconnect"));
	


}
void ofApp::exit(){
	releaseFrameBuffer();
}

//--------------------------------------------------------------
void ofApp::update(){
	ofSetWindowTitle(ofToString(ofGetFrameRate(), 2));


	static unsigned long start=0;

	for(int i=0;i<MAX_NUM_UNIVERSES;i++){
		bSend[i]=false;
		bReceive[i]=false;
	}

	if(mode==PLAY){

		status="PLAY";
		numUniverses=MAX_NUM_UNIVERSES;
		if(!bPause)
		currentFrame = currentFrame+1;
		if(currentFrame>maxFrame-1)
			currentFrame = 0;
		sendFrame(frames[currentFrame]);
	}
	
	if(mode==THROUGH || mode==REC){

		//start=ofGetElapsedTimeMicros();
		//status = "No Art-net receive";

		int ret;
		numUniverses = 0;
		for(int i=0;i<MAX_NUM_UNIVERSES;i++)
			bReceive[i] = false;
		char buffer[ARTNET_PACKET_SIZE];

		while(ret=udpReceiver.Receive(buffer,ARTNET_PACKET_SIZE) >= 0){
			//parseArtnetDMX(buffer);
			storePacket(buffer);
			numUniverses++;
			if(numUniverses>=10)break;
		}

		if(numUniverses>0){


			if(bThrough)
			sendFrame(frameBuffer);
		

			if(bRecording){
			
				status="REC";
				storeFrame(frameBuffer,currentFrame);
				if(!bPause)
				currentFrame = currentFrame+1;
				if(currentFrame>maxFrame-1)
					bRecording=false;
				
			}else{
				status="THROUGH";
			}
		}
		//printf("%d\n",ofGetElapsedTimeMicros()-start);
	}
}

void ofApp::storePacket(const char* artnetPacket){

	int universe = getUniverse(artnetPacket);
	if(universe<MAX_NUM_UNIVERSES){
		char * dst = frameBuffer + universe* ARTNET_PACKET_SIZE;
		memcpy(dst,artnetPacket,ARTNET_PACKET_SIZE);
		bReceive[universe] = true;
	}
}

void ofApp::storeFrame(char* frameBuffer,int frameNum){

	memcpy(frames[frameNum],frameBuffer,FRAME_SIZE);

}

void ofApp::allocateFrameBuffer(){
	
	for(int i=0;i<MAX_FRAME_NUM;i++){
		char* frame = new char[FRAME_SIZE];
		frames.push_back(frame);
	}
}
void ofApp::releaseFrameBuffer(){
	
	for(int i=0;i<frames.size();i++){
		delete [] frames[i];
	}
}


void ofApp::parseArtnetDMX(char* artnetPacket){


		// Check that packetID is "Art-Net" else ignore
		for (int i = 0 ; i < 9 ; i++)
		{
			if (artnetPacket[i] != ART_NET_ID[i])
				return ;
		}
        
		int opcode = artnetPacket[8] | artnetPacket[9] << 8; 
		int sequence;
		int incomingUniverse;
		int dmxDataLength;
		char* data = artnetPacket+ ART_DMX_START;

		if (opcode == ART_DMX){

			sequence = artnetPacket[12];
			incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;  
			dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;
			
		}

		//printf("Sequence:%3d\tUniverse:%2d\n",sequence,incomingUniverse);

		bReceive[incomingUniverse]=true;

}
int ofApp::getUniverse(const char* artnetPacket){
	return artnetPacket[14] | artnetPacket[15] << 8 ;
}


void ofApp::receivePacket(char* artnetPacket){
	bReceive[getUniverse(frameBuffer)]=true;
}

void ofApp::sendPacket(int index,char* packet){

	for(int i=0;i<PACKET_SIZE;i++){
		packet[i]= (unsigned char)((unsigned char)packet[i]*bright/255.0);
	}
	
	udpSender[index].SendAll((char*)packet,PACKET_SIZE);
	bSend[index]=true;

}
void ofApp::sendFrame(char * frameBuffer){

	unsigned char *buffer = new unsigned char [PACKET_SIZE];

	int firstLen = ARTNET_PACKET_SIZE-ART_DMX_START-2;
	int secondLen = PACKET_SIZE-firstLen;

	for(int i=0;i<numUniverses;i++){

		int sendTo = i/2;

		if(i%2==0){ // universe 0,2,4,6,8
			char *src =(char*)frameBuffer+i*ARTNET_PACKET_SIZE+ART_DMX_START;
			memcpy(buffer,src,firstLen);
		}else{ //universe  1,3,5,7,9
		    char *src = (char*)frameBuffer+i*ARTNET_PACKET_SIZE+ART_DMX_START;
			memcpy(buffer+firstLen,src,secondLen);
			sendPacket(sendTo,(char*)buffer);
			//for(int i=0;i<NUM_LEDS;i++){
			//	printf("DATA:%d R:%d G:%d B:%d\n",i,buffer[i*3],buffer[i*3+1],buffer[i*3+2]);
			//}
		}
	};

	delete [] buffer;

}

void ofApp::sendTestPacket(int index,ofColor color){
	
	char buffer[PACKET_SIZE];
	
	for(int i=0;i<PACKET_SIZE;i+=3){
		buffer[i]=color.r;
		buffer[i+1]=color.g;
		buffer[i+2]=color.b;
	}
	numUniverses=10;
	sendPacket(index,buffer);

	printf("LED BORAD[%d] R%d G%d B%d\n",index,color.r,color.g,color.b);
}

void ofApp::onTest(){

	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		sendTestPacket(i,ofColor(30,30,30));
		ofSleepMillis(500);
		sendTestPacket(i,ofColor(0,0,0));
	}
}

void ofApp::onReconnect(){

	//Receiver Setup
	if(!udpReceiver.Create()){
		printf("UDP Receiver Error:INVALID SOCKET\n");
	};
	if(udpReceiver.Bind(ARTNET_PORT)<0){
		printf("UDP Error:can't bind\n");
	};
	udpReceiver.SetNonBlocking(true);

	printf("UDP Receive Buffer Size:%d\n",udpReceiver.GetReceiveBufferSize());

	//Sender Setup
	for(int i=0;i<NUM_REMOTE_DEVICES;i<i++){
		if(!udpSender[i].Create()){
			printf("UDP Sender[%d] Error:INVALID SOCKET\n",i);
		}
		else if(udpSender[i].Connect(remoteIP[i].c_str(),ARTNET_PORT)){;
			udpSender[i].SetNonBlocking(true);	
			printf("UDP Sender[%d] Connected to:%s\n",i,remoteIP[i].c_str());
		}else{
			printf("UDP Sender[%d] can't connect to:%s\n",i,remoteIP[i].c_str());
		}
	}


}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor(0,0,0));

	for(int i=0;i<MAX_NUM_UNIVERSES;i++){
		if(bReceive[i])ofSetColor(ofColor(0,0,200));
		else ofSetColor(ofColor(50));
		ofCircle(ofPoint(15*(i+1),15*1),5);
	}

	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		if(bSend[i])ofSetColor(ofColor(0,200,0));
		else ofSetColor(ofColor(50));
		ofCircle(ofPoint(15*(i+1),15*2),5);
	}		
	
	gui.draw();
	
}

void ofApp::onRec(bool &bRec){

	

	//on Recoding
	if(bRec){
		mode=REC;
		currentFrame.setFillColor(ofColor(200,0,0));

	}else{
		mode=THROUGH;
		currentFrame.setFillColor(ofColor(120));
		int max = currentFrame;
		maxFrame = max;
		currentFrame=0;
	}

	if(bPlaying)bPlaying=false;
}

void ofApp::onPlay(bool &bPlay){

	if(bRecording)bRecording=false;

	//on Play
	if(bPlay){
		mode=PLAY;
		currentFrame.setFillColor(ofColor(0,200,0));
		currentFrame = 0;

	}else{
		mode=THROUGH;
		currentFrame.setFillColor(ofColor(120));
		currentFrame=0;
	}
}


void ofApp::onChangeFPS(int &val){

	ofSetFrameRate(val);
}

void ofApp::onChangeBright(int &val){
	
	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		ofxOscMessage msg;
		msg.setAddress("/bright");
		msg.addIntArg(val);
		oscSender[i].sendMessage(msg);
	}
	printf("Brightness %d\n",val);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key){
	
	case 'q':
		maxFrame = maxFrame-1;
		break;
	case 'w':
		maxFrame = maxFrame+1;
		break;
	case 'p':
		bPause = !bPause;
		break;
	case 's':
		currentFrame = currentFrame+1;
		break;
	case 'a':
		currentFrame = currentFrame-1;
		break;
	
	}


}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
