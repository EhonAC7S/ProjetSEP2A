#include "pch.h"
#include "Chronometre.h"

// On va ici cacher/encapsuler le fonctionnement du chrono : Cr�ation, Start, Reset, Stop 
// Doc sur clock : https://www.tutorialspoint.com/c_standard_library/c_function_clock.htm
//http://stackoverflow.com/questions/15092504/how-to-time-a-function-in-milliseconds-without-boosttimer
Chronometre::Chronometre()
{
	begin = clock() - clock(); // On triche pour se dire que le chrono, bah au d�but il est � z�ro quand on le construit (on triche pour construire le '0' en clock_t)
	elapsedtime = clock() - clock();
	alreadyStarted = false; // A la cr�ation le chrono tourne pas lol
}


Chronometre::~Chronometre() // Petit destructeur oklm
{
}


void Chronometre::StopChrono()
{
	if (alreadyStarted) { // Si le chrono est allum�, on va vraiment faire un stop, et mettre le bool � false, sinon faire un Stop n'a pas de sens si le chrono tourne pas
		elapsedtime = elapsedtime + clock() - begin; // Ben ducoup on va stocker le temps �coul� depuis le start, jusqu'� maintenant et le rajouter au buffer 
		alreadyStarted = false; // Ben l� le chrono est stop� donc �teint
	}
}

void Chronometre::StartRestartChrono()
{
	if (!alreadyStarted) { //Si le chrono �tait � z�ro ou en pause, en lui redonne le point de d�part maintenant 
						//(le temps pr�c�dement �coul� entre le start et le stop pr�cedent s'il n'y a 
						//pas eu de reset entre temps est d�ja sauv� dans le buffer elapsedTime
		begin = clock();
	}
}

float Chronometre::getTimeElapsed()
{
	if (alreadyStarted) {
		return (float) (elapsedtime + (clock() - begin)) / CLOCKS_PER_SEC; // On va avoir le temps en secondes (avec d�cimales, 
																//en float et faire les conversion 
																//ailleurs #LeViewerFeraSontTaff
	}
	else return (float) elapsedtime; // Si le chrono est coup�, ce qui est dans le buffer suffit pour l'affichage
}

void Chronometre::resetChrono()
{
	if (alreadyStarted) { // Le bouton reset ne fait rien si le chrono tourne (calqu� sur le fonctionnement habituel des chronos)
		begin = clock() - clock(); // Faudrait voir si le constructeur clock_t() donnerait pas 0 par hasard
		elapsedtime = clock() - clock();
		alreadyStarted = false;
	}
}
