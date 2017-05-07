#pragma once

using namespace std;

ref class Chronometre sealed
{
public:
	Chronometre();
	virtual ~Chronometre();
	void StartRestartChrono();
	void StopChrono();
	float getTimeElapsed();
	void resetChrono();


private:

	clock_t begin;
	clock_t elapsedtime;
	bool alreadyStarted; // Sauvegarde le temps écoulé entre les start et stop
	
};

