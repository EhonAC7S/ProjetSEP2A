//
// SuspensionManager.cpp
// Implémentation de la classe SuspensionManager
//

#include "pch.h"
#include "SuspensionManager.h"

#include <algorithm>

using namespace Chrono::Common;

using namespace concurrency;
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::FileProperties;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;

Map<String^, Object^>^ SuspensionManager::_sessionState = ref new Map<String^, Object^>();

const wchar_t* SuspensionManager::sessionStateFilename = L"_sessionState.dat";

std::vector<WeakReference> SuspensionManager::_registeredFrames;

DependencyProperty^ SuspensionManager::FrameSessionStateKeyProperty = nullptr;
DependencyProperty^ SuspensionManager::FrameSessionBaseKeyProperty = nullptr;
DependencyProperty^ SuspensionManager::FrameSessionStateProperty = nullptr;

void SuspensionManager::RegisterDependencyProperties()
{
	if (FrameSessionStateProperty == nullptr)
	{
		FrameSessionStateProperty = DependencyProperty::RegisterAttached("_FrameSessionStateProperty",
			TypeName(IMap<String^, Object^>::typeid), TypeName(SuspensionManager::typeid), nullptr);
	}

	if (FrameSessionBaseKeyProperty == nullptr)
	{
		FrameSessionBaseKeyProperty = DependencyProperty::RegisterAttached("_FrameSessionBaseKeyProperty",
			TypeName(String::typeid), TypeName(SuspensionManager::typeid), nullptr);
	}

	if (FrameSessionStateKeyProperty == nullptr)
	{
		FrameSessionStateKeyProperty = DependencyProperty::RegisterAttached("_FrameSessionStateKeyProperty",
			TypeName(String::typeid), TypeName(SuspensionManager::typeid), nullptr);
	}
}

class ObjectSerializeHelper
{
public:
	// Codes utilisés pour identifier les types sérialisés
	enum StreamTypes {
		NullPtrType = 0,

		// Types IPropertyValue pris en charge
		UInt8Type, UInt16Type, UInt32Type, UInt64Type, Int16Type, Int32Type, Int64Type,
		SingleType, DoubleType, BooleanType, Char16Type, GuidType, StringType,

		// Autres types pris en charge
		StringToObjectMapType,

		// Valeur de marqueur utilisée pour garantir l'intégrité du flux
		MapEndMarker
	};
	static String^ ReadString(DataReader^ reader);
	static IMap<String^, Object^>^ ReadStringToObjectMap(DataReader^ reader);
	static Object^ ReadObject(DataReader^ reader);
	static void WriteString(DataWriter^ writer, String^ string);
	static void WriteProperty(DataWriter^ writer, IPropertyValue^ propertyValue);
	static void WriteStringToObjectMap(DataWriter^ writer, IMap<String^, Object^>^ map);
	static void WriteObject(DataWriter^ writer, Object^ object);
};

/// <summary>
/// Permet d'accéder à l'état de session global pour la session active.  Cet état est sérialisé par
/// <see cref="SaveAsync"/> et restauré par <see cref="RestoreAsync"/> qui requiert des valeurs
/// de type suivant : valeurs boxed, y compris des entiers, des valeurs uniques et doubles à virgule flottante,
/// des caractères larges, des valeurs booléennes, des chaînes et des GUID, ou Map<String^, Object^> où les valeurs de mappage sont
/// soumises aux mêmes contraintes.  L'état de session doit être aussi compact que possible.
/// </summary>
IMap<String^, Object^>^ SuspensionManager::SessionState()
{
	return _sessionState;
}

/// <summary>
/// Inscrit une instance de <see cref="Frame"/> pour que son historique de navigation puisse être enregistré dans
/// et restauré à partir de <see cref="SessionState"/>.  Les frames doivent être inscrits une seule fois,
/// immédiatement après la création s'ils doivent participer à la gestion de l'état de session.  Lors de
/// l'inscription, si l'état a déjà été restauré pour la clé spécifiée,
/// l'historique de navigation est immédiatement restauré.  Les appels ultérieurs de
/// <see cref="RestoreAsync(String)"/> restaurent également l'historique de navigation.
/// </summary>
/// <param name="frame">Instance dont l'historique de navigation doit être géré par
/// <see cref="SuspensionManager"/></param>
/// <param name="sessionStateKey">Clé unique dans <see cref="SessionState"/> utilisée pour
/// enregistrer des informations relatives à la navigation.</param>
/// <param name="sessionBaseKey">Clé facultative qui identifie le type de session.
/// Peut servir à effectuer la distinction entre plusieurs scénarios de lancement d'application.</param>
void SuspensionManager::RegisterFrame(Frame^ frame, String^ sessionStateKey, String^ sessionBaseKey)
{
	if (frame->GetValue(FrameSessionStateKeyProperty) != nullptr)
	{
		throw ref new FailureException("Frames can only be registered to one session state key");
	}

	if (frame->GetValue(FrameSessionStateProperty) != nullptr)
	{
		throw ref new FailureException("Frames must be either be registered before accessing frame session state, or not registered at all");
	}

	if (sessionBaseKey != nullptr)
	{
		frame->SetValue(FrameSessionBaseKeyProperty, sessionBaseKey);
		sessionStateKey = sessionBaseKey + "_" + sessionStateKey;
	}

	// Utilise une propriété de dépendance pour associer la clé de session à un frame et conserver une liste des frames dont
	// l'état de navigation doit être géré
	frame->SetValue(FrameSessionStateKeyProperty, sessionStateKey);
	_registeredFrames.insert(_registeredFrames.begin(), WeakReference(frame));

	// Vérifie si l'état de navigation peut être restauré
	RestoreFrameNavigationState(frame);
}

/// <summary>
/// Dissocie un <see cref="Frame"/> précédemment inscrit par <see cref="RegisterFrame"/>
/// de <see cref="SessionState"/>.  Tout état de navigation précédemment capturé sera
/// supprimé.
/// </summary>
/// <param name="frame">Instance dont l'historique de navigation ne doit plus être
/// géré.</param>
void SuspensionManager::UnregisterFrame(Frame^ frame)
{
	// Supprime l'état de session et supprime le frame de la liste des frames dont l'état de navigation
	// sera enregistré (avec les références faibles qui ne sont plus accessibles)
	auto key = safe_cast<String^>(frame->GetValue(FrameSessionStateKeyProperty));
	if (SessionState()->HasKey(key))
	{
		SessionState()->Remove(key);
	}
	_registeredFrames.erase(
		std::remove_if(_registeredFrames.begin(), _registeredFrames.end(), [=](WeakReference& e)
	{
		auto testFrame = e.Resolve<Frame>();
		return testFrame == nullptr || testFrame == frame;
	}),
		_registeredFrames.end()
		);
}

/// <summary>
/// Permet de stocker l'état de session associé au <see cref="Frame"/> spécifié.
/// L'état de session des frames inscrits précédemment à l'aide de <see cref="RegisterFrame"/> est
/// enregistré et restauré automatiquement dans le cadre du
/// <see cref="SessionState"/> global.  Les frames qui ne sont pas inscrits ont un état transitoire
/// qui peut néanmoins être utile lors de la restauration de pages qui ont été supprimées du
/// cache de navigation.
/// </summary>
/// <remarks>Les applications peuvent utiliser <see cref="NavigationHelper"/> pour gérer
/// l'état spécifique aux pages, au lieu de gérer directement l'état de session de frames.</remarks>
/// <param name="frame">Instance pour laquelle l'état de session est requis.</param>
/// <returns>Collection d'états pour lequel est utilisé le même mécanisme de sérialisation que
/// <see cref="SessionState"/>.</returns>
IMap<String^, Object^>^ SuspensionManager::SessionStateForFrame(Frame^ frame)
{
	auto frameState = safe_cast<IMap<String^, Object^>^>(frame->GetValue(FrameSessionStateProperty));

	if (frameState == nullptr)
	{
		auto frameSessionKey = safe_cast<String^>(frame->GetValue(FrameSessionStateKeyProperty));
		if (frameSessionKey != nullptr)
		{
			// Les frames inscrits reflètent l'état de session correspondant
			if (!_sessionState->HasKey(frameSessionKey))
			{
				_sessionState->Insert(frameSessionKey, ref new Map<String^, Object^>());
			}
			frameState = safe_cast<IMap<String^, Object^>^>(_sessionState->Lookup(frameSessionKey));
		}
		else
		{
			// Les frames non inscrits ont un état transitoire
			frameState = ref new Map<String^, Object^>();
		}
		frame->SetValue(FrameSessionStateProperty, frameState);
	}
	return frameState;
}

void SuspensionManager::RestoreFrameNavigationState(Frame^ frame)
{
	auto frameState = SessionStateForFrame(frame);
	if (frameState->HasKey("Navigation"))
	{
		frame->SetNavigationState(safe_cast<String^>(frameState->Lookup("Navigation")));
	}
}

void SuspensionManager::SaveFrameNavigationState(Frame^ frame)
{
	auto frameState = SessionStateForFrame(frame);
	frameState->Insert("Navigation", frame->GetNavigationState());
}

/// <summary>
/// Enregistre le <see cref="SessionState"/> actuel.  Toutes les instances de <see cref="Frame"/>
/// inscrites avec <see cref="RegisterFrame"/> conservent également leur
/// pile de navigation actuelle, ce qui permet à leur <see cref="Page"/> active
/// d'enregistrer son état.
/// </summary>
/// <returns>Tâche asynchrone qui reflète quand l'état de session a été enregistré.</returns>
task<void> SuspensionManager::SaveAsync(void)
{
	// Enregistre l'état de navigation pour tous les frames inscrits
	for (auto && weakFrame : _registeredFrames)
	{
		auto frame = weakFrame.Resolve<Frame>();
		if (frame != nullptr) SaveFrameNavigationState(frame);
	}

	// Sérialise l'état de session de manière synchrone pour éviter un accès asynchrone à un
	// état partagé
	auto sessionData = ref new InMemoryRandomAccessStream();
	auto sessionDataWriter = ref new DataWriter(sessionData->GetOutputStreamAt(0));
	ObjectSerializeHelper::WriteObject(sessionDataWriter, _sessionState);

	// Après la capture synchrone de l'état de session, commence le processus asynchrone
	// d'écriture du résultat sur le disque
	return task<unsigned int>(sessionDataWriter->StoreAsync()).then([=](unsigned int)
	{
		return ApplicationData::Current->LocalFolder->CreateFileAsync(StringReference(sessionStateFilename),
			CreationCollisionOption::ReplaceExisting);
	})
		.then([=](StorageFile^ createdFile)
	{
		return createdFile->OpenAsync(FileAccessMode::ReadWrite);
	})
		.then([=](IRandomAccessStream^ newStream)
	{
		return RandomAccessStream::CopyAsync(
			sessionData->GetInputStreamAt(0), newStream->GetOutputStreamAt(0));
	})
		.then([=](UINT64 copiedBytes)
	{
		(void) copiedBytes; // Paramètre non utilisé
		return;
	});
}

/// <summary>
/// Restaure le <see cref="SessionState"/> précédemment enregistré.  Toutes les instances de <see cref="Frame"/>
/// inscrites avec <see cref="RegisterFrame"/> restaurent également leur état de navigation
/// précédent, ce qui permet à leur <see cref="Page"/> active de restaurer son
/// d'une application.
/// </summary>
/// <param name="sessionBaseKey">Clé facultative qui identifie le type de session.
/// Peut servir à effectuer la distinction entre plusieurs scénarios de lancement d'application.</param>
/// <returns>Tâche asynchrone qui reflète quand un état de session a été lu.  Le
/// contenu de <see cref="SessionState"/> ne doit pas être pris en compte tant que cette tâche n'est pas
/// achevée.</returns>
task<void> SuspensionManager::RestoreAsync(String^ sessionBaseKey)
{
	_sessionState->Clear();

	task<StorageFile^> getFileTask(ApplicationData::Current->LocalFolder->GetFileAsync(StringReference(sessionStateFilename)));
	return getFileTask.then([=](StorageFile^ stateFile)
	{
		task<BasicProperties^> getBasicPropertiesTask(stateFile->GetBasicPropertiesAsync());
		return getBasicPropertiesTask.then([=](BasicProperties^ stateFileProperties)
		{
			auto size = unsigned int(stateFileProperties->Size);
			if (size != stateFileProperties->Size) throw ref new FailureException("Session state larger than 4GB");
			task<IRandomAccessStreamWithContentType^> openReadTask(stateFile->OpenReadAsync());
			return openReadTask.then([=](IRandomAccessStreamWithContentType^ stateFileStream)
			{
				auto stateReader = ref new DataReader(stateFileStream);
				return task<unsigned int>(stateReader->LoadAsync(size)).then([=](unsigned int bytesRead)
				{
					(void) bytesRead; // Paramètre non utilisé
					// Désérialise l'état de session
					Object^ content = ObjectSerializeHelper::ReadObject(stateReader);
					_sessionState = (Map<String^, Object^>^)content;

					// Restaure l'état enregistré des frames inscrits
					for (auto && weakFrame : _registeredFrames)
					{
						auto frame = weakFrame.Resolve<Frame>();
						if (frame != nullptr && safe_cast<String^>(frame->GetValue(FrameSessionBaseKeyProperty)) == sessionBaseKey)
						{
							frame->ClearValue(FrameSessionStateProperty);
							RestoreFrameNavigationState(frame);
						}
					}
				}, task_continuation_context::use_current());
			});
		});
	});
}

#pragma region Object serialization for a known set of types

void ObjectSerializeHelper::WriteString(DataWriter^ writer, String^ string)
{
	writer->WriteByte(StringType);
	writer->WriteUInt32(writer->MeasureString(string));
	writer->WriteString(string);
}

void ObjectSerializeHelper::WriteProperty(DataWriter^ writer, IPropertyValue^ propertyValue)
{
	switch (propertyValue->Type)
	{
	case PropertyType::UInt8:
		writer->WriteByte(StreamTypes::UInt8Type);
		writer->WriteByte(propertyValue->GetUInt8());
		return;
	case PropertyType::UInt16:
		writer->WriteByte(StreamTypes::UInt16Type);
		writer->WriteUInt16(propertyValue->GetUInt16());
		return;
	case PropertyType::UInt32:
		writer->WriteByte(StreamTypes::UInt32Type);
		writer->WriteUInt32(propertyValue->GetUInt32());
		return;
	case PropertyType::UInt64:
		writer->WriteByte(StreamTypes::UInt64Type);
		writer->WriteUInt64(propertyValue->GetUInt64());
		return;
	case PropertyType::Int16:
		writer->WriteByte(StreamTypes::Int16Type);
		writer->WriteUInt16(propertyValue->GetInt16());
		return;
	case PropertyType::Int32:
		writer->WriteByte(StreamTypes::Int32Type);
		writer->WriteUInt32(propertyValue->GetInt32());
		return;
	case PropertyType::Int64:
		writer->WriteByte(StreamTypes::Int64Type);
		writer->WriteUInt64(propertyValue->GetInt64());
		return;
	case PropertyType::Single:
		writer->WriteByte(StreamTypes::SingleType);
		writer->WriteSingle(propertyValue->GetSingle());
		return;
	case PropertyType::Double:
		writer->WriteByte(StreamTypes::DoubleType);
		writer->WriteDouble(propertyValue->GetDouble());
		return;
	case PropertyType::Boolean:
		writer->WriteByte(StreamTypes::BooleanType);
		writer->WriteBoolean(propertyValue->GetBoolean());
		return;
	case PropertyType::Char16:
		writer->WriteByte(StreamTypes::Char16Type);
		writer->WriteUInt16(propertyValue->GetChar16());
		return;
	case PropertyType::Guid:
		writer->WriteByte(StreamTypes::GuidType);
		writer->WriteGuid(propertyValue->GetGuid());
		return;
	case PropertyType::String:
		WriteString(writer, propertyValue->GetString());
		return;
	default:
		throw ref new InvalidArgumentException("Unsupported property type");
	}
}

void ObjectSerializeHelper::WriteStringToObjectMap(DataWriter^ writer, IMap<String^, Object^>^ map)
{
	writer->WriteByte(StringToObjectMapType);
	writer->WriteUInt32(map->Size);
	for (auto && pair : map)
	{
		WriteObject(writer, pair->Key);
		WriteObject(writer, pair->Value);
	}
	writer->WriteByte(MapEndMarker);
}

void ObjectSerializeHelper::WriteObject(DataWriter^ writer, Object^ object)
{
	if (object == nullptr)
	{
		writer->WriteByte(NullPtrType);
		return;
	}

	auto propertyObject = dynamic_cast<IPropertyValue^>(object);
	if (propertyObject != nullptr)
	{
		WriteProperty(writer, propertyObject);
		return;
	}

	auto mapObject = dynamic_cast<IMap<String^, Object^>^>(object);
	if (mapObject != nullptr)
	{
		WriteStringToObjectMap(writer, mapObject);
		return;
	}

	throw ref new InvalidArgumentException("Unsupported data type");
}

String^ ObjectSerializeHelper::ReadString(DataReader^ reader)
{
	int length = reader->ReadUInt32();
	String^ string = reader->ReadString(length);
	return string;
}

IMap<String^, Object^>^ ObjectSerializeHelper::ReadStringToObjectMap(DataReader^ reader)
{
	auto map = ref new Map<String^, Object^>();
	auto size = reader->ReadUInt32();
	for (unsigned int index = 0; index < size; index++)
	{
		auto key = safe_cast<String^>(ReadObject(reader));
		auto value = ReadObject(reader);
		map->Insert(key, value);
	}
	if (reader->ReadByte() != StreamTypes::MapEndMarker)
	{
		throw ref new InvalidArgumentException("Invalid stream");
	}
	return map;
}

Object^ ObjectSerializeHelper::ReadObject(DataReader^ reader)
{
	auto type = reader->ReadByte();
	switch (type)
	{
	case StreamTypes::NullPtrType:
		return nullptr;
	case StreamTypes::UInt8Type:
		return reader->ReadByte();
	case StreamTypes::UInt16Type:
		return reader->ReadUInt16();
	case StreamTypes::UInt32Type:
		return reader->ReadUInt32();
	case StreamTypes::UInt64Type:
		return reader->ReadUInt64();
	case StreamTypes::Int16Type:
		return reader->ReadInt16();
	case StreamTypes::Int32Type:
		return reader->ReadInt32();
	case StreamTypes::Int64Type:
		return reader->ReadInt64();
	case StreamTypes::SingleType:
		return reader->ReadSingle();
	case StreamTypes::DoubleType:
		return reader->ReadDouble();
	case StreamTypes::BooleanType:
		return reader->ReadBoolean();
	case StreamTypes::Char16Type:
		return (char16_t) reader->ReadUInt16();
	case StreamTypes::GuidType:
		return reader->ReadGuid();
	case StreamTypes::StringType:
		return ReadString(reader);
	case StreamTypes::StringToObjectMapType:
		return ReadStringToObjectMap(reader);
	default:
		throw ref new InvalidArgumentException("Unsupported property type");
	}
}

#pragma endregion
