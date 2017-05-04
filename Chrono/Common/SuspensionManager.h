//
// SuspensionManager.h
// Déclaration de la classe SuspensionManager
//

#pragma once

namespace Chrono
{
	namespace Common
	{
		/// <summary>
		/// SuspensionManager capture l'état de session global pour simplifier la gestion de la durée de vie des processus
		/// pour une application.  Notez que l'état de session sera automatiquement supprimé sous diverses
		/// conditions ; il doit uniquement être utilisé pour stocker des informations qu'il est utile
		/// d'utiliser dans plusieurs sessions, mais qui doivent être supprimées lorsqu'une application cesse de fonctionner ou est
		/// mise à jour.
		/// </summary>
		class SuspensionManager sealed
		{
		public:
			static void RegisterDependencyProperties();
			static void RegisterFrame(Windows::UI::Xaml::Controls::Frame^ frame, Platform::String^ sessionStateKey, Platform::String^ sessionBaseKey = nullptr);
			static void UnregisterFrame(Windows::UI::Xaml::Controls::Frame^ frame);
			static concurrency::task<void> SaveAsync();
			static concurrency::task<void> RestoreAsync(Platform::String^ sessionBaseKey = nullptr);
			static Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ SessionState();
			static Windows::Foundation::Collections::IMap<Platform::String^, Platform::Object^>^ SessionStateForFrame(
				Windows::UI::Xaml::Controls::Frame^ frame);

		private:
			static void RestoreFrameNavigationState(Windows::UI::Xaml::Controls::Frame^ frame);
			static void SaveFrameNavigationState(Windows::UI::Xaml::Controls::Frame^ frame);

			static Platform::Collections::Map<Platform::String^, Platform::Object^>^ _sessionState;
			static const wchar_t* sessionStateFilename;

			static std::vector<Platform::WeakReference> _registeredFrames;
			static Windows::UI::Xaml::DependencyProperty^ FrameSessionStateKeyProperty;
			static Windows::UI::Xaml::DependencyProperty^ FrameSessionBaseKeyProperty;
			static Windows::UI::Xaml::DependencyProperty^ FrameSessionStateProperty;
		};
	}
}
