#pragma once
#include <guiddef.h>
#include <Unknwn.h>
#include <winrt/base.h>

// forward declare namespaces we alias
namespace winrt {
	namespace Windows {
		namespace ApplicationModel {}
		namespace Foundation {
			namespace Collections {}
			namespace Numerics {}
		}
		namespace Graphics::Effects {}
		namespace UI {
			namespace Composition {}
			namespace Xaml {
				namespace Controls {}
				namespace Hosting {}
			}
		}
	}
}

// alias some long namespaces for convenience
namespace wam = winrt::Windows::ApplicationModel;
namespace wf = winrt::Windows::Foundation;
namespace wfc = wf::Collections;
namespace wfn = wf::Numerics;
namespace wge = winrt::Windows::Graphics::Effects;
namespace wuc = winrt::Windows::UI::Composition;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxh = wux::Hosting;
