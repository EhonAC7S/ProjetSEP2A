//
// SampleDataSource.h
// Déclaration des classes SampleDataSource, SampleDataGroup, SampleDataItem et SampleDataCommon
//

#pragma once

// Le modèle de données défini par ce fichier sert d'exemple représentatif d'un modèle fortement typé
// modèle.  Les noms de propriétés choisis correspondent aux liaisons de données dans les modèles d'élément standard.
//
// Les applications peuvent utiliser ce modèle comme point de départ et le modifier à leur convenance, ou le supprimer complètement et
// le remplacer par un autre correspondant à leurs besoins. L'utilisation de ce modèle peut vous permettre d'améliorer votre application 
// réactivité en lançant la tâche de chargement des données dans le code associé à App.xaml lorsque l'application 
// est démarrée pour la première fois.

namespace Chrono
{
	namespace Data
	{
		ref class SampleDataGroup; // Résolvez la relation circulaire entre SampleDataItem et SampleDataGroup

		/// <summary>
		/// Modèle de données d'élément générique.
		/// </summary>
		[Windows::UI::Xaml::Data::Bindable]
		public ref class SampleDataItem sealed : public Windows::UI::Xaml::Data::ICustomPropertyProvider
		{
		public:
			property Platform::String^ UniqueId { Platform::String^ get(); }
			property Platform::String^ Title { Platform::String^ get(); }
			property Platform::String^ Subtitle { Platform::String^ get(); }
			property Platform::String^ Description { Platform::String^ get(); }
			property Platform::String^ Content { Platform::String^ get(); }
			property Platform::String^ ImagePath { Platform::String^ get(); }

			// L'implémentation d'ICustomPropertyProvider fournit une représentation sous forme de chaîne de l'objet à utiliser comme nom Automation à des fins d'accessibilité
			virtual Windows::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name);
			virtual Windows::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, Windows::UI::Xaml::Interop::TypeName type);
			virtual Platform::String^ GetStringRepresentation();
			property Windows::UI::Xaml::Interop::TypeName Type { virtual Windows::UI::Xaml::Interop::TypeName get(); }

		internal:
			SampleDataItem(Platform::String^ uniqueId, Platform::String^ title, Platform::String^ subtitle, Platform::String^ imagePath,
				Platform::String^ description, Platform::String^ content);

		private:
			Platform::String^ _uniqueId;
			Platform::String^ _title;
			Platform::String^ _subtitle;
			Platform::String^ _imagePath;
			Platform::String^ _description;
			Platform::String^ _content;
		};

		/// <summary>
		/// Modèle de données de groupe générique.
		/// </summary>
		[Windows::UI::Xaml::Data::Bindable]
		public ref class SampleDataGroup sealed : public Windows::UI::Xaml::Data::ICustomPropertyProvider
		{
		public:
			property Platform::String^ UniqueId { Platform::String^ get(); }
			property Platform::String^ Title { Platform::String^ get(); }
			property Platform::String^ Subtitle { Platform::String^ get(); }
			property Platform::String^ Description { Platform::String^ get(); }
			property Windows::Foundation::Collections::IObservableVector<SampleDataItem^>^ Items
			{
				Windows::Foundation::Collections::IObservableVector<SampleDataItem^>^ get();
			}
			property Platform::String^ ImagePath { Platform::String^ get(); }

			// L'implémentation d'ICustomPropertyProvider fournit une représentation sous forme de chaîne de l'objet à utiliser comme nom Automation à des fins d'accessibilité
			virtual Windows::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name);
			virtual Windows::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, Windows::UI::Xaml::Interop::TypeName type);
			virtual Platform::String^ GetStringRepresentation();
			property Windows::UI::Xaml::Interop::TypeName Type { virtual Windows::UI::Xaml::Interop::TypeName get(); }

		internal:
			SampleDataGroup(Platform::String^ uniqueId, Platform::String^ title, Platform::String^ subtitle, Platform::String^ imagePath,
				Platform::String^ description);

		private:
			Platform::String^ _uniqueId;
			Platform::String^ _title;
			Platform::String^ _subtitle;
			Platform::String^ _imagePath;
			Platform::String^ _description;
			Platform::Collections::Vector<SampleDataItem^>^ _items;
		};

		/// <summary>
		/// Crée une collection de groupes et d'éléments dont le contenu est lu à partir d'un fichier json statique.
		/// 
		/// SampleDataSource initialise avec les données lues à partir d'un fichier json statique dans 
		/// projet.  Elle fournit des exemples de données à la fois au moment de la conception et de l'exécution.
		/// </summary>
		[Windows::UI::Xaml::Data::Bindable]
		public ref class SampleDataSource sealed
		{
		public:
			property Windows::Foundation::Collections::IObservableVector<SampleDataGroup^>^ Groups
			{
				Windows::Foundation::Collections::IObservableVector<SampleDataGroup^>^ get();
			}

		internal:
			SampleDataSource();
			static concurrency::task<Windows::Foundation::Collections::IIterable<SampleDataGroup^>^> GetGroups();
			static concurrency::task<SampleDataGroup^> GetGroup(Platform::String^ uniqueId);
			static concurrency::task<SampleDataItem^> GetItem(Platform::String^ uniqueId);

		private:
			concurrency::task_completion_event<void> _loadCompletionEvent;
			Platform::Collections::Vector<SampleDataGroup^>^ _groups;
			static concurrency::task<void> Init();

			static SampleDataSource^ _sampleDataSource;
		};
	}
}
