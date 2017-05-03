//
// App.xaml.cpp
// Implémentation de la classe App.
//

#include "pch.h"
#include "ItemPage.xaml.h"
#include "PivotPage.xaml.h"

using namespace ChronoGPSBasselin_Cesaro;
using namespace ChronoGPSBasselin_Cesaro::Common;

using namespace concurrency;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Navigation;

// Pour plus d'informations sur le modèle Application Pivot, consultez la page http://go.microsoft.com/fwlink/?LinkID=391641

/// <summary>
/// Initialise l'objet d'application de singleton. Il s'agit de la première ligne du code créé
/// à être exécutée. Elle correspond donc à l'équivalent logique de main() ou WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);

	PivotPage::RegisterDependencyProperties();
	ItemPage::RegisterDependencyProperties();
	SuspensionManager::RegisterDependencyProperties();
}

/// <summary>
/// Invoqué lorsque l'application est lancée normalement par l'utilisateur final.  D'autres points d'entrée
/// sont utilisés lorsque l'application est lancée pour ouvrir un fichier spécifique, pour afficher
/// des résultats de recherche, etc.
/// </summary>
/// <param name="e">Détails concernant la requête et le processus de lancement.</param>
void App::OnLaunched(LaunchActivatedEventArgs^ e)
{

#if _DEBUG
	// Affichez des informations de profilage graphique lors du débogage.
	if (IsDebuggerPresent())
	{
		// Afficher les compteurs de fréquence des trames actuels
		DebugSettings->EnableFrameRateCounter = true;
	}
#endif

	auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

	// Ne répétez pas l'initialisation de l'application lorsque la fenêtre comporte déjà du contenu,
	// assurez-vous juste que la fenêtre est active
	if (rootFrame == nullptr)
	{
		// Crée un frame qui joue le rôle de contexte de navigation et l'associe à
		// une clé SuspensionManager
		rootFrame = ref new Frame();

		// Associez le frame à une clé SuspensionManager.
		SuspensionManager::RegisterFrame(rootFrame, "AppFrame");

		// TODO: modifier cette valeur à une taille de cache qui contient à votre application.
		rootFrame->CacheSize = 1;

		// Définir la page par défaut
		rootFrame->Language = Windows::Globalization::ApplicationLanguages::Languages->GetAt(0);

		auto prerequisite = task<void>([](){});
		if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
		{
			// Restaure l'état de session enregistré uniquement lorsque cela est approprié, en planifiant
			// les étapes de lancement finales une fois la restauration terminée
			prerequisite = SuspensionManager::RestoreAsync();
		}

		prerequisite.then([=](task<void> prerequisite)
		{
			try
			{
				prerequisite.get();
			}
			catch (Platform::Exception^)
			{
				// Un problème est survenu lors de la restauration de l'état.
				// Partez du principe que l'état est absent et continuez.
				__debugbreak();
			}

			if (rootFrame->Content == nullptr)
			{
				// Supprime la navigation tourniquet pour le démarrage.
				if (rootFrame->ContentTransitions != nullptr)
				{
					_transitions = ref new TransitionCollection();
					for (auto transition : rootFrame->ContentTransitions)
					{
						_transitions->Append(transition);
					}
				}

				rootFrame->ContentTransitions = nullptr;
				_firstNavigatedToken = rootFrame->Navigated += ref new NavigatedEventHandler(this, &App::RootFrame_FirstNavigated);

				// Quand la pile de navigation n'est pas restaurée, accédez à la première page,
				// puis configurez la nouvelle page en transmettant les informations requises en tant que
				// paramètre
				rootFrame->Navigate(PivotPage::typeid, e->Arguments);
			}

			// Placez le frame dans la fenêtre active
			Window::Current->Content = rootFrame;

			// Vérifiez que la fenêtre actuelle est active
			Window::Current->Activate();

		}, task_continuation_context::use_current());
	}
	else
	{
		if (rootFrame->Content == nullptr)
		{
			// Quand la pile de navigation n'est pas restaurée, accédez à la première page,
			// puis configurez la nouvelle page en transmettant les informations requises en tant que
			// paramètre
			if (!rootFrame->Navigate(PivotPage::typeid, e->Arguments))
			{
				throw ref new FailureException("Failed to create initial page");
			}
		}

		// Vérifiez que la fenêtre actuelle est active
		Window::Current->Activate();
	}
}

/// <summary>
/// Restaure les transitions de contenu une fois l'application lancée.
/// </summary>
void App::RootFrame_FirstNavigated(Object^ sender, NavigationEventArgs^ e)
{
	auto rootFrame = safe_cast<Frame^>(sender);

	TransitionCollection^ newTransitions;
	if (_transitions == nullptr)
	{
		newTransitions = ref new TransitionCollection();
		newTransitions->Append(ref new NavigationThemeTransition());
	}
	else
	{
		newTransitions = _transitions;
	}

	rootFrame->ContentTransitions = newTransitions;
	rootFrame->Navigated -= _firstNavigatedToken;
}

/// <summary>
/// Appelé lorsque l'exécution de l'application est suspendue. L'état de l'application est enregistré
/// sans savoir si l'application pourra se fermer ou reprendre sans endommager
/// le contenu de la mémoire.
/// </summary>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// Paramètre non utilisé
	(void) e;		// Paramètre non utilisé

	auto deferral = e->SuspendingOperation->GetDeferral();
	SuspensionManager::SaveAsync().then([=]()
	{
		deferral->Complete();
	});
}