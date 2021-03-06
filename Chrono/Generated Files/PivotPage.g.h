﻿

#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class Pivot;
                ref class ListView;
                ref class Button;
                ref class TextBlock;
            }
        }
    }
}
namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                namespace Maps {
                    ref class MapControl;
                }
            }
        }
    }
}

namespace Chrono
{
    partial ref class PivotPage : public ::Windows::UI::Xaml::Controls::Page, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
    
        private: ::Windows::UI::Xaml::Controls::Pivot^ pivot;
        private: ::Windows::UI::Xaml::Controls::ListView^ list;
        private: ::Windows::UI::Xaml::Controls::Button^ resetPoi;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ textBlock1;
        private: ::Windows::UI::Xaml::Controls::Maps::MapControl^ myMap;
        private: ::Windows::UI::Xaml::Controls::Button^ button;
        private: ::Windows::UI::Xaml::Controls::Button^ button1;
        private: ::Windows::UI::Xaml::Controls::Button^ button2;
        private: ::Windows::UI::Xaml::Controls::Button^ button3;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ textBlock;
    };
}

