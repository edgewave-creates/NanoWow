#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 64

class Block {
private:
	int currentValue;
	bool hasMap = false;

protected:
	int fromLow, fromHigh, toLow, toHigh;

public:
	int next() {
		return currentValue;
	}

	int setCurrentValue(int val) {
		if (hasMap) {
			currentValue = map(val, fromLow, fromHigh, toLow, toHigh);
		} else {
			currentValue = val;
		}
	}

	void setMap(int fromLow, int fromHigh, int toLow, int toHigh) {
		this->fromLow = fromLow;
		this->fromHigh = fromHigh;
		this->toLow = toLow;
		this->toHigh = toHigh;

		hasMap = true;
	}

	virtual void updateControl() {}

	virtual void updateAudio() {}

};

class SinewaveOscillator : public Block {
protected:
	Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>* osc;
	Block* frequencyControl;
	Block* detuneControl;

public:

	SinewaveOscillator() {
		osc = new Oscil<SIN2048_NUM_CELLS, AUDIO_RATE>(SIN2048_DATA);
	}

	void setFrequencyControl(Block* B) {
		frequencyControl = B;
	}

	void setDetuneControl(Block* B) {
		detuneControl = B;
	}

	void updateControl() {
		osc->setFreq(frequencyControl->next() + (detuneControl ? detuneControl->next() : 0));
	}

	void updateAudio() {
		setCurrentValue(osc->next());
	}

};

class FixedInt : public Block {
public:
	FixedInt(int val) {
		setCurrentValue(val);
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
		setCurrentValue(source->next() * gainControl->next());
	}

};

class AnalogIn : public Block {
protected:
	int pin;

public:
	AnalogIn(int pin, int toLow, int toHigh) {
		this->pin = pin;
		setMap(0, 1023, toLow, toHigh);
	}

	void updateControl() {
		setCurrentValue(mozziAnalogRead(pin));
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
AnalogIn freq(A2, 200, 2000);

Amplifier amp;
AnalogIn gain(A3, 0, 255);

SinewaveOscillator lfo;
AnalogIn lfoFreq(A4, 0, 20);


Rack rack;

void setup() {
	startMozzi(CONTROL_RATE);

	// Add Blocks to Rack
	rack.addBlock(&OscA);
	rack.addBlock(&amp);
	rack.addBlock(&gain);
	rack.addBlock(&freq);
	rack.addBlock(&lfo);
	rack.addBlock(&lfoFreq);

	// Initialize Blocks
	// lfo.setMap(-128, 127, -50, 50);

	// Wire Blocks Together
	OscA.setFrequencyControl(&freq);
	lfo.setFrequencyControl(&lfoFreq);
	OscA.setDetuneControl(&lfo);
	amp.setGainControl(&gain);
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
