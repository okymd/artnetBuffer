#include "ofApp.h"

////Arduino IP's
const string remoteIP[NUM_REMOTE_DEVICES] ={
	"192.168.11.200",
	"192.168.11.201",
	"192.168.11.202",
	"192.168.11.203",
	"192.168.11.204"
};

const string DEVICE_ID[5] = {"A","B","C","D","E" };

//
const int HEADER_HEIGHT = 60;

//--------------------------------------------------------------
void ofApp::setup(){


	//Window Position
	ofSetWindowPosition(10,40);
	ofSetWindowTitle("Artnet Buffer for Matr");


	
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


	//Buffer allocate
	frameBuffer = new char[ARTNET_PACKET_SIZE*MAX_NUM_UNIVERSES];
	
	allocateFrameBuffer();

	mode = REC;
	

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
	gui.setPosition(0,HEADER_HEIGHT);

	gui.add(labelFPS.setup("FPS:","FPS"));
	gui.add(labelStatus.setup("STATUS:","NO SIGNAL"));
	gui.add(bRecording.setup("Rec",false));
	gui.add(bPlaying.setup("Play",false));
	gui.add(bThrough.setup("Through",true));
	gui.add(bPause.setup("Pause",false));
	gui.add(currentFrame.setup("Frame",0,0,MAX_FRAME_NUM));
	gui.add(startFrame.setup("Start Frame",0,0,MAX_FRAME_NUM));
	gui.add(endFrame.setup("End Frame",MAX_FRAME_NUM,1,MAX_FRAME_NUM));
	gui.add(bright.setup("Bright",255,0,255));
	gui.add(fps.setup("FPS",75,25,100));
	gui.add(btnTriple.setup("FPSx3"));
	gui.add(btnDouble.setup("FPSx2"));
	gui.add(btnNormal.setup("FPSx1"));	
	gui.add(btnTest.setup("LED TEST"));
	gui.add(btnReconnect.setup("Reconnect"));
	gui.add(bShowMask.setup("Show Mask",false));
	gui.add(bApplyMask.setup("Apply Mask",true));
	gui.add(maskStatus.setup("Display","Mask status"));


	//Slider
	ofColor dark = ofColor(0);
	ofColor light = ofColor(120);
//	startFrame.setBackgroundColor(light);
//	startFrame.setFillColor(dark);
//	startFrame.setBorderColor(light);
//	endFrame.setBackgroundColor(dark);
//	endFrame.setFillColor(light);
//	endFrame.setBorderColor(light);


	//pixel
	sendImg.allocate(24,40,OF_IMAGE_COLOR);
	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		maskImg[i].allocate(24,8,OF_IMAGE_COLOR);
		maskImg[i].loadImage("mask"+ofToString(i)+".bmp");
	}
	currentShowMask=0;


	font.loadFont("verdana.ttf", 8);


}
void ofApp::exit(){
	releaseFrameBuffer();

	sendImg.clear();
	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		maskImg[i].clear();
	}
}

//--------------------------------------------------------------
void ofApp::update(){

	labelFPS = ofToString(ofGetFrameRate(),2);


	static unsigned long start=0;

	for(int i=0;i<MAX_NUM_UNIVERSES;i++){
		bSend[i]=false;
		bReceive[i]=false;
	}

	if(mode==PLAY){

		labelStatus="PLAY";
		numRecvUniverses=MAX_NUM_UNIVERSES;
		if(!bPause)
		currentFrame = currentFrame+1;
		if(currentFrame>endFrame-1)
			currentFrame = (int)startFrame;
		sendFrame(frames[currentFrame]);
	}
	
	if(mode==REC){

		//start=ofGetElapsedTimeMicros();
		//status = "No Art-net receive";

		int ret;
		numRecvUniverses = 0;

		for(int i=0;i<MAX_NUM_UNIVERSES;i++)
			bReceive[i] = false;
		char buffer[ARTNET_PACKET_SIZE];

		while(ret=udpReceiver.Receive(buffer,ARTNET_PACKET_SIZE) >= 0){
			//parseArtnetDMX(buffer);
			storePacket(buffer);
			numRecvUniverses++;
			if(numRecvUniverses>MAX_NUM_UNIVERSES)break;
		}

		if(numRecvUniverses>0){


			if(bThrough)
				sendFrame(frameBuffer);
			else labelStatus="";


			if(bRecording){
			
				labelStatus="REC";
				storeFrame(frameBuffer,currentFrame);
				if(!bPause)
				currentFrame = currentFrame+1;
				if(currentFrame>MAX_FRAME_NUM)bRecording=false;
				
			}else{
				labelStatus="THROUGH";
			}
		}else{
			labelStatus="---";
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

	for(int i=0;i<NUM_REMOTE_DEVICES;i++)
		sendBuffer[i] = new char[PACKET_SIZE];

}
void ofApp::releaseFrameBuffer(){
	
	for(int i=0;i<frames.size();i++){
		delete [] frames[i];
	}

	for(int i=0;i<NUM_REMOTE_DEVICES;i++)
		delete sendBuffer[i];

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
	unsigned char *image = new unsigned char [24*40*3];

	int firstLen = ARTNET_PACKET_SIZE-ART_DMX_START-2;
	int secondLen = PACKET_SIZE-firstLen;


	for(int i=0;i<numRecvUniverses;i++){

		int sendTo = i/2;

		if(i%2==0){ // universe 0,2,4,6,8
			char *src =(char*)frameBuffer+i*ARTNET_PACKET_SIZE+ART_DMX_START;
			memcpy(buffer,src,firstLen);
		}else{ //universe  1,3,5,7,9
		    char *src = (char*)frameBuffer+i*ARTNET_PACKET_SIZE+ART_DMX_START;
			memcpy(buffer+firstLen,src,secondLen);

			//send Packet
			if(bApplyMask)
				doMask((char*)buffer,(char*)maskImg[sendTo].getPixels(),(char*)buffer);
			sendPacket(sendTo,(char*)buffer);
			memcpy(sendBuffer[sendTo],buffer,PACKET_SIZE);
		}
	};
	
	updateSendPixel();

	delete [] buffer;
	delete [] image;

}

void ofApp::doMask(char * src,char* mask,char* dst){

	for(int i=0;i<PACKET_SIZE;i++){
		dst[i] = src[i] & mask[i];
	}
}

void ofApp::sendTestPacket(int index,ofColor color){
	
	char buffer[PACKET_SIZE];
	
	for(int i=0;i<PACKET_SIZE;i+=3){
		buffer[i]=color.r;
		buffer[i+1]=color.g;
		buffer[i+2]=color.b;
	}
	numRecvUniverses=10;
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
	if(!udpReceiver.Close()){
		udpReceiver.Create();
		printf("UDP Receiver :Create SOCKET\n");
	};
	if(udpReceiver.Bind(ARTNET_PORT)<0){
		printf("UDP Error:can't bind\n");
	};
	udpReceiver.SetNonBlocking(true);

	printf("UDP Receive Buffer Size:%d\n",udpReceiver.GetReceiveBufferSize());

	//Sender Setup
	for(int i=0;i<NUM_REMOTE_DEVICES;i<i++){
		if(!udpSender[i].Close()){
			udpSender[i].Create();
			printf("UDP Sender[%d] :Create SOCKET\n",i);
		}
		if(udpSender[i].Connect(remoteIP[i].c_str(),ARTNET_PORT)){;
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

	ofPushStyle();
	for(int i=0;i<MAX_NUM_UNIVERSES;i++){
		if(bReceive[i])ofSetColor(ofColor(0,0,200));
		else ofSetColor(ofColor(50));
		ofCircle(ofPoint(20*(i+1),20*1),8);
	}

	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		if(bSend[i])ofSetColor(ofColor(0,200,0));
		else ofSetColor(ofColor(50));
		ofCircle(ofPoint(20*(i+1),20*2),8);
	}		
	ofPopStyle();
	
	
	gui.draw();


	ofTranslate(0,gui.getPosition().y+gui.getShape().height);
	if(bShowMask){		
		drawPixel((char*)maskImg[currentShowMask].getPixels());
		maskStatus = DEVICE_ID[currentShowMask] + ":Mask Image";
	}else{
		drawPixel(sendBuffer[currentShowMask]);
		if(bApplyMask)maskStatus = DEVICE_ID[currentShowMask] + ":Masked Pixel";
		else maskStatus = DEVICE_ID[currentShowMask] + ":Not Masked Pixel";
	}

	ofTranslate(0,10*10);
	drawKeymap();
}

void ofApp::drawPixel(char * pixel){

	

	ofPushStyle();
	int dx=10;
	int dy=10;
	int w = 24;
	int h = 8;
	
	for(int y=0;y<h;y++){
		for(int x=0;x<w;x++){
			char r =pixel[3*(x+y*w)];
			char g =pixel[3*(x+y*w)+1];
			char b =pixel[3*(x+y*w)+2];
			ofSetColor(ofColor(r,g,b));
			if(x+24*y==activePixel){
				ofSetColor(ofColor(255,!g,!b));
			}
			ofRect(dx*x,dy*y,dx,dy);
		}
	}
	ofPopStyle();
	ofPushStyle();
	ofSetLineWidth(1);
	ofSetColor(ofColor(40));
	for(int y=0;y<h+1;y++){
		ofLine(0,dy*y,dx*w,dy*y);
	}
	for(int x=0;x<w+1;x++){
		ofLine(dx*x,0,dx*x,dy*h);
	}
	ofPopStyle();



}


void ofApp::updateSendPixel(){

	char* temp = new char[PACKET_SIZE*NUM_REMOTE_DEVICES];

	for(int i=0;i<NUM_REMOTE_DEVICES;i++){
		memcpy(temp+i*PACKET_SIZE,sendBuffer[i],PACKET_SIZE);
	}

	sendImg.setFromPixels((const unsigned char*)temp,24,40,OF_IMAGE_COLOR);

	delete temp;

}

void ofApp::onRec(bool &bRec){

	//on Recoding
	if(bRec){
		mode=REC;
		currentFrame = 0;
		currentFrame.setFillColor(ofColor(200,0,0));
	}else{
		currentFrame.setFillColor(ofColor(120));
		endFrame = (int)currentFrame;
		currentFrame = (int)startFrame;
	}

	if(bPlaying)bPlaying=false;
}

void ofApp::onPlay(bool &bPlay){

	if(bRecording)bRecording=false;

	//on Play
	if(bPlay){
		mode=PLAY;
		currentFrame.setFillColor(ofColor(0,200,0));
		currentFrame = (int)startFrame;

	}else{
		mode=REC;
		currentFrame.setFillColor(ofColor(120));
		currentFrame=(int)startFrame;
	}
}


void ofApp::onChangeFPS(int &val){

	ofSetFrameRate(val);
}

void ofApp::onChangeBright(int &val){
	
	//for(int i=0;i<NUM_REMOTE_DEVICES;i++){
	//	ofxOscMessage msg;
	//	msg.setAddress("/bright");
	//	msg.addIntArg(val);
	//	oscSender[i].sendMessage(msg);
	//}
	//printf("Brightness %d\n",val);
}


void ofApp::drawKeymap(){

	ofSetColor(ofColor(200));

	string keys = "[KEY MAP]\n";
	keys +="q/w:start Frame\n";
	keys +="a/s:current Frame\n";
	keys +="z/x:end Frame\n";
	keys +="p:PAUSE\n";
	keys +="UP/DOWN: change show device\n";

	font.drawString(keys,0,0);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key){
	
	case 'q':
		startFrame = startFrame-1;
		break;
	case 'w':
		startFrame = startFrame+1;
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
	case 'z':
		endFrame = endFrame-1;
		break;
	case 'x':
		endFrame = endFrame+1;
		break;

	case OF_KEY_DOWN:
		currentShowMask++;
		if(currentShowMask>NUM_REMOTE_DEVICES-1)
			currentShowMask=0;
		maskImg[currentShowMask].loadImage("mask"+ofToString(currentShowMask)+".bmp");
		break;
	case OF_KEY_UP:
		currentShowMask--;
		if(currentShowMask<0)
			currentShowMask=NUM_REMOTE_DEVICES-1;
		maskImg[currentShowMask].loadImage("mask"+ofToString(currentShowMask)+".bmp");
		break;

	}


}

int ofApp::getActive(int x,int y){

	int org_x=0;
	int org_y=gui.getHeight()+gui.getPosition().y;
	int dx=10;
	int dy=10;


	for(int i=0;i<NUM_LEDS;i++){
		int _x = i%24;
		int _y = i/24;
		ofRectangle pixRect = ofRectangle(_x*dx+org_x,_y*dy+org_y,dx,dy);
		if(pixRect.inside(x,y)){
			return i;
		};	
	}

	return -1;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

	
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	activePixel=getActive(x,y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	int i=getActive(x,y);
	if(i>=0){
		int size = 24*8*3;
		char* buf= new char[size];
		activePixel=i;
		memcpy(buf,(char*)maskImg[currentShowMask].getPixels(),size);
		//set color
		int p= activePixel*3;
		if(buf[p]!=0)memset(&buf[p],0,3);
		else memset(&buf[p],255,3);
		//replace
		maskImg[currentShowMask].setFromPixels((const unsigned char*)buf,24,8,OF_IMAGE_COLOR);
		//save bitmap
		maskImg[currentShowMask].saveImage("mask"+ofToString(currentShowMask)+".bmp");
		delete[] buf;
	}
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
