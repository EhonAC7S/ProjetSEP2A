//
// ItemPage.xaml.h
// Déclaration de la classe ItemPage.
//

#pragma once

#include "ItemPage.g.h"

namespace ChronoGPSBasselin_Cesaro
{
	/// <summary>
	/// Page qui affiche les détails d'un élément appartenant à un groupe.
	/// </summary>
	public ref class ItemPage sealed
	{
	public:
		ItemPage();

		static void RegisterDependencyProperties();
		static property Windows::UI::Xaml::DependencyProperty^ DefaultViewModelProperty
		{
			Windows::UI::Xaml::DependencyProperty^ get() { return _defaultViewModelProperty; }
		}

		static property Windows::UI::Xaml::DependencyProperty^ NavigationHelperProperty
		{
			Windows::UI::Xaml::DependencyProperty^ get() { return _navigationHelperProperty; }
		}

		/// <summary>
		/// Obtient le <see cref="NavigationHelper"/> associé à ce <see cref="Page"/>.
		/// </summary>
		property Common::NavigationHelper^ NavigationHelper
		{
			Common::NavigationHelper^ get();
		}

		/// <summary>
		/// Obtient le modèle d'affichage pour ce <see cref="Page"/>.
		/// Cela peut être remplacé par un modèle d'affichage fortement typé.
		/// </summary>
		property Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^ DefaultViewModel
		{
			Windows::Foundation::Collections::IObservableMap<Platform::String^, Platform::Object^>^  get();
		}

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
		virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void NavigationHelper_LoadState(Platform::Object^ sender, Common::LoadStateEventArgs^ e);
		void NavigationHelper_SaveState(Platform::Object^ sender, Common::SaveStateEventArgs^ e);

		static Windows::UI::Xaml::DependencyProperty^ _defaultViewModelProperty;
		static Windows::UI::Xaml::DependencyProperty^ _navigationHelperProperty;
	};
}
