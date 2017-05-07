#include "pch.h"
#include "ChronoDisplayer.h"
#include <math.h>


ChronoDisplayer::ChronoDisplayer()
{
}

/*string ChronoDisplayer::affichageChrono(float elapsedTime)
{
	int millisec = (int) (elapsedTime - floor(elapsedTime))*1000; //On recup les décimales de elapsedTime et on miltiplie par 1000 pour avoir les millisec en int
	int sec = (int) floor(elapsedTime); // On recupere le reste (total des secondes)
	int min = sec / 60; //Divions euclidienne -> nb de paquet de 60 sec = nb de min
	sec = sec % 60; //Reste de la div est le nb de secondes à afficher
	return to_string(min) + string(":") + to_string(sec) + string(":") + to_string(millisec); // On concatene le tout
}*/
