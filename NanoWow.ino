#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 64

class Block {
protected:
	int currentValue;

public:
	int next() {
		return currentValue;
	}

	virtual void updateControl() {}

	virtual void updateAudio() {}

};

class SinewaveOscillator : public Block {
protected:
	Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>* osc;

public:

	SinewaveOscillator() {
		osc = new Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA);
	}

	void setFreq(int F) {
		osc->setFreq(F);
	}

	void updateAudio() {
		currentValue = osc->next();
	}

};

class FixedInt : public Block {
public:
	FixedInt() {
		currentValue = 120;
	}
};

class Amplifier : public Block {
protected:
	Block* gainControl;
	Block* source;

public:
	void setSource(Block* B) {
		source = B;
	}

	void setGainControl(Block* B) {
		gainControl = B;
	}

	void updateAudio() {
		currentValue = source->next() * gainControl->next();
	}

};

#define MAX_BLOCKS 20

class Rack : public Block {
protected:
	Block* blocks[MAX_BLOCKS];
	int nextSlot = 0;

public:
	Block* addBlock(Block* B) {
		blocks[nextSlot++] = B;
		return B;
	}

	void updateControl() {
		for (int i = 0; i < nextSlot; i++) {
			blocks[i]->updateControl();
		}
	}

	void updateAudio() {
		for (int i = 0; i < nextSlot; i++) {
			blocks[i]->updateAudio();
		}
	}
};

SinewaveOscillator OscA;
SinewaveOscillator V;
Amplifier amp;
Rack rack;

void setup() {
	startMozzi(CONTROL_RATE);

	// Add Blocks to Rack
	rack.addBlock(&OscA);
	rack.addBlock(&amp);
	rack.addBlock(&V);

	// Initialize Blocks

	OscA.setFreq(880);
	V.setFreq(2);

	// Wire Blocks Together
	amp.setGainControl(&V);
	amp.setSource(&OscA);
}

void loop() {
	audioHook();
}

void updateControl() {
	rack.updateControl();
}

AudioOutput_t updateAudio() {
	rack.updateAudio();

	return MonoOutput::from16Bit(amp.next());
}
