//
// PivotPage.xaml.cpp
// Implémentation de la classe PivotPage.
//

#include "pch.h"
#include "PivotPage.xaml.h"
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <ctime>

using namespace Chrono;
using namespace Chrono::Common;
using namespace Chrono::Data;

using namespace concurrency;
using namespace Platform;
using namespace Platform::Collections;

using namespace Windows::ApplicationModel::Resources;
using namespace Windows::Devices::Geolocation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Controls::Maps;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

Windows::Foundation::IAsyncOperation<Windows::Devices::Geolocation::Geoposition^>^ m_getOperation;

using namespace std::chrono;
using namespace std;


// CONSTANTE : 
const double DISTANCE_UNITE_LATITUDE_LONGITUDE = 79000; // On a au niveau de Bordeau, environs 1 latitude = 79 km

//variable
mutex ChronoMutex;
mutex SynchroMutex;
chrono::system_clock::time_point SysTime;
double latitude;
double longitude;
chrono::system_clock::duration elapsedTime;
long long millisec;
long long sec;
long long minu;
long long heure;
bool reset;
bool stop;
bool start;
bool hasNoCaptureReg;
double latitudeReg;
double longitudeReg;
long long timeReg;
int nbOfSpeedCapture;
double meanOfSpeed;
Platform::String^ heureStr;
Platform::String^ minuStr;
Platform::String^ secStr;
Platform::String^ millisecstr;


// Structure contenant les données liées à la mesure du temps et de la position.

static UINT MajChrono() {
	while (1) {
		lock_guard<mutex> locker(ChronoMutex);
		lock_guard<mutex> locker1(SynchroMutex);
		auto elapsed1 = chrono::system_clock::now() - SysTime;
		elapsed1 = elapsed1 + elapsedTime;
		millisec = (chrono::duration_cast<std::chrono::milliseconds>(elapsed1).count()/10) % 100;
		sec = (chrono::duration_cast<std::chrono::seconds>(elapsed1).count()) % 60;
		minu = (chrono::duration_cast<std::chrono::minutes>(elapsed1).count()) % 60;
		heure = (chrono::duration_cast<std::chrono::hours>(elapsed1 % 60).count()) % 60;
	}
	return 0;
}

static UINT geo() {
	while (1) {
		lock_guard<mutex> locker(SynchroMutex);
		Geolocator^ geolocator = ref new Geolocator();
		m_getOperation = nullptr;
		m_getOperation = geolocator->GetGeopositionAsync();
		m_getOperation->Completed = ref new AsyncOperationCompletedHandler<Geoposition^>(
			[=](IAsyncOperation<Geoposition^>^ asyncOperation, AsyncStatus status) mutable
		{
			if (status != AsyncStatus::Error)
			{
				Geoposition^ geoposition = asyncOperation->GetResults();
				latitude = geoposition->Coordinate->Latitude;
				longitude = geoposition->Coordinate->Longitude;
			}
		});

	}
	return 0;
}

PivotPage::PivotPage() {

	InitializeComponent();
	NavigationCacheMode = Navigation::NavigationCacheMode::Required;
	_resourceLoader = ResourceLoader::GetForCurrentView(L"Resources");
	auto navigationHelper = ref new Common::NavigationHelper(this);
	navigationHelper->LoadState += ref new LoadStateEventHandler(this, &PivotPage::NavigationHelper_LoadState);
	navigationHelper->SaveState += ref new SaveStateEventHandler(this, &PivotPage::NavigationHelper_SaveState);
	SetValue(_defaultViewModelProperty, ref new Platform::Collections::Map<String^, Object^>(std::less<String^>()));
	SetValue(_navigationHelperProperty, navigationHelper);
	ChronoMutex.lock();
	SynchroMutex.lock();
	SynchroMutex.unlock();
	reset = false;
	stop = true;
	hasNoCaptureReg = true;
	nbOfSpeedCapture = 0;
	meanOfSpeed = 0;
	elapsedTime = chrono::system_clock::now() - chrono::system_clock::now();
	thread ThGPS(geo);
	ThGPS.detach();
	thread ThChrono(MajChrono);
	ThChrono.detach();
	//initialisation des boutons
	this->button2->IsEnabled = false;
	this->button3->IsEnabled = false;
	this->button1->IsEnabled = false;
	DispatcherTimer^ timer = ref new DispatcherTimer;
	timer->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &Chrono::PivotPage::DispatcherTimer_Tick);
	TimeSpan t;
	t.Duration = 10000;// expressed in 100s of nanoseconds;
	timer->Interval = t;
	timer->Start();

}



DependencyProperty^ PivotPage::_navigationHelperProperty = nullptr;
DependencyProperty^ PivotPage::_defaultViewModelProperty = nullptr;

void PivotPage::RegisterDependencyProperties()
{
	if (_navigationHelperProperty == nullptr)
	{
		_navigationHelperProperty = DependencyProperty::Register("NavigationHelper",
			TypeName(Common::NavigationHelper::typeid), TypeName(PivotPage::typeid), nullptr);
	}

	if (_defaultViewModelProperty == nullptr)
	{
		_defaultViewModelProperty =
			DependencyProperty::Register("DefaultViewModel",
				TypeName(IObservableMap<String^, Object^>::typeid), TypeName(PivotPage::typeid), nullptr);
	}
}


IObservableMap<String^, Object^>^ PivotPage::DefaultViewModel::get()
{
	return safe_cast<IObservableMap<String^, Object^>^>(GetValue(_defaultViewModelProperty));
}

NavigationHelper^ PivotPage::NavigationHelper::get()
{
	return safe_cast<Common::NavigationHelper^>(GetValue(_navigationHelperProperty));
}

void PivotPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedTo(e);
}

void PivotPage::OnNavigatedFrom(NavigationEventArgs^ e)
{
	NavigationHelper->OnNavigatedFrom(e);
}

/// <summary>
/// Remplit la page à l'aide du contenu passé lors de la navigation. Tout état enregistré est également
/// fourni lorsqu'une page est recréée à partir d'une session antérieure.
/// </summary>
/// <param name="sender">
/// La source de l'événement, en général <see cref="NavigationHelper"/>.
/// </param>
/// <param name="e">Données d'événement qui fournissent le paramètre de navigation transmis à
/// <see cref="Frame->Navigate(Type, Object)"/> lors de la requête initiale de cette page et
/// un dictionnaire d'état conservé par cette page durant une session
/// antérieure. L'état n'aura pas la valeur Null lors de la première visite de la page.</param>

void PivotPage::NavigationHelper_LoadState(Object^ sender, LoadStateEventArgs^ e)
{
	(void)sender;	// Paramètre non utilisé
	(void)e;		// Paramètre non utilisé

	SampleDataSource::GetGroup(L"Group-1").then([this](SampleDataGroup^ sampleDataGroup)
	{
		DefaultViewModel->Insert(GetFirstGroupName(), sampleDataGroup);
	}, task_continuation_context::use_current());
}

/// <summary>
/// Conserve l'état associé à cette page en cas de suspension de l'application ou de
/// suppression de la page du cache de navigation. Les valeurs doivent être conforme aux
/// exigences en matière de sérialisation de <see cref="SuspensionManager.SessionState"/>.
/// </summary>
/// <param name="sender">
/// La source de l'événement, en général <see cref="NavigationHelper"/>.
/// </param>
/// <param name="e">
/// Données d'événement qui fournissent un dictionnaire vide à remplir avec un état sérialisable.
/// </param>
void PivotPage::NavigationHelper_SaveState(Object^ sender, SaveStateEventArgs^ e)
{
	// TODO: enregistrer l'état unique de la page ici.
}

void Chrono::PivotPage::textBlock_SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}

void PivotPage::SecondPivot_Loaded(Object^ sender, RoutedEventArgs ^e)
{
	(void)sender;	// Paramètre non utilisé
	(void)e;		// Paramètre non utilisé

					// TODO: créer un modèle de données approprié pour le domaine posant problème afin de remplacer les exemples de données.
	SampleDataSource::GetGroup(L"Group-2").then([this](SampleDataGroup^ sampleDataGroup)
	{
		DefaultViewModel->Insert(GetSecondGroupName(), sampleDataGroup);
	}, task_continuation_context::use_current());
}

void Chrono::PivotPage::DispatcherTimer_Tick(Platform::Object^ sender, Platform::Object^ e)
{
	if (stop) {
		//On ne fait rien 
	}
	else {
		heureStr = heure.ToString();
		minuStr = minu.ToString();
		secStr = sec.ToString();
		millisecstr = millisec.ToString();
		if (heure < 10) {
			heureStr = "0" + heureStr;
		}
		if (minu < 10) {
			minuStr = "0" + minuStr;
		}
		if (sec < 10) {
			secStr = "0" + secStr;
		}
		if (millisec < 10) {
			millisecstr = "0" + millisecstr;
		}

		textBlock->Text = ""+ heureStr +":"+ minuStr +":"+ secStr +"."+ millisecstr;
	}
}


//start
void Chrono::PivotPage::button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->button2->IsEnabled = true;
	if (reset || stop) {
		this->button->IsEnabled = false;
		this->button1->IsEnabled = true;
		this->button3->IsEnabled = false;
		stop = false;
		reset = false;
		SysTime = chrono::system_clock::now();
		ChronoMutex.unlock();
	}
}

//stop
void Chrono::PivotPage::button1_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->button->IsEnabled = true;
	this->button1->IsEnabled = false;
	this->button3->IsEnabled = true;
	stop = true;
	elapsedTime = chrono::system_clock::now() - SysTime + elapsedTime;
	ChronoMutex.lock();

}

//Reset
void Chrono::PivotPage::button3_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->button3->IsEnabled = false;
	this->button1->IsEnabled = false;
	textBlock->Text = "00:00:00.00";
	reset = true;
	elapsedTime = chrono::system_clock::now() - chrono::system_clock::now();
	
}

//capture
void Chrono::PivotPage::button2_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (sec>=3 || minu>0 || heure>0) {
		BasicGeoposition geoPoint = BasicGeoposition();
		geoPoint.Latitude = latitude;
		geoPoint.Longitude = longitude;
		Geopoint^ CurrentPoint = ref new Geopoint(geoPoint);
		myMap->Center = CurrentPoint;
		myMap->ZoomLevel = 12;
		SynchroMutex.lock();
		if (hasNoCaptureReg) {
			list->Items->Append("LAT : " + latitude + " LON : " + longitude + " TEMPS : " + heureStr + ":" + minuStr + ":" + secStr + "." + millisecstr);
		}
		else {
			double speed = sqrt(pow((latitude - latitudeReg),2) + pow((longitude - longitudeReg),2)) * DISTANCE_UNITE_LATITUDE_LONGITUDE / ((double) timeReg);
			list->Items->Append("LAT : " + latitude + " LON : " + longitude + " TEMPS : " + heureStr + ":" + minuStr + ":" + secStr + "." + millisecstr + " SPEED : " + speed +  "m/s");
			meanOfSpeed = (meanOfSpeed * nbOfSpeedCapture + speed) / (nbOfSpeedCapture + 1);
			nbOfSpeedCapture = nbOfSpeedCapture + 1;
			textBlock1->Text = "Moyenne : " + meanOfSpeed + "m/s";
		}
		hasNoCaptureReg = false;
		latitudeReg = latitude;
		longitudeReg = longitude;
		timeReg = sec + minu * 60 + heure * 3600; //en seconde.
		SynchroMutex.unlock();
	}
	
}


void Chrono::PivotPage::button4_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	list->Items->Clear();
	hasNoCaptureReg = true;
	textBlock1->Text = "Moyenne : ";
	nbOfSpeedCapture = 0;
	meanOfSpeed = 0;
}

