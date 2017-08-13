#include "headers.hpp"

#include "far_api.hpp"

#include "py_boolean.hpp"
#include "py_import.hpp"
#include "py_integer.hpp"
#include "py_list.hpp"
#include "py_string.hpp"
#include "py_tuple.hpp"
#include "py_uuid.hpp"

#include "py_common.hpp"

#include "error_handling.hpp"

#include "python.hpp"

using namespace py::literals;

namespace far_api_implementation
{
	static const auto& fsf()
	{
		return far_api::get().fsf();
	}

	static const auto& psi()
	{
		return far_api::get().psi();
	}

	template<typename T, typename converter = decltype(py::cast<T>)>
	static auto list_to_vector(const py::list& List, const converter& Converter = py::cast<T>)
	{
		const auto Size = List.size();
		std::vector<T> Result;
		Result.reserve(Size);
		for (size_t i = 0; i != Size; ++i)
		{
			Result.emplace_back(Converter(List[i]));
		}
		return Result;
	}

	static auto get_args(const char* Name, PyObject* RawArgs, size_t Count)
	{
		const auto Args = py::cast<py::tuple>(py::object::from_borrowed(RawArgs));
		if (Args.size() != Count)
			throw MAKE_PYGIN_EXCEPTION(Name + ": wrong number of arguments (expected: "s + std::to_string(Count) + ", actual: "s + std::to_string(Args.size()) + ")"s);
		return Args;
	}

	static PyObject* GetMsg(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 2);

		const auto PluginId = py::cast<UUID>(Args[0]);
		const auto MsgId = py::cast<intptr_t>(Args[1]);

		return py::string(psi().GetMsg(&PluginId, MsgId)).release();
	}

	static PyObject* Message(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 7);

		const auto PluginId = py::cast<UUID>(Args[0]);
		const auto Id = py::cast<UUID>(Args[1]);
		const auto Flags = py::cast<FARMESSAGEFLAGS>(Args[2]);
		const auto HelpTopic = py::cast<std::wstring>(Args[3]);
		const auto Title = py::cast<std::wstring>(Args[4]);
		const auto Items = list_to_vector<std::wstring>(py::cast<py::list>(Args[5]));
		const auto Buttons = list_to_vector<std::wstring>(py::cast<py::list>(Args[6]));

		std::vector<const wchar_t*> AllItems;
		AllItems.reserve(1 + Items.size() + Buttons.size());
		AllItems.emplace_back(Title.data());
		std::transform(Items.cbegin(), Items.cend(), std::back_inserter(AllItems), [](const auto& i) { return i.data(); });
		std::transform(Buttons.cbegin(), Buttons.cend(), std::back_inserter(AllItems), [](const auto& i) { return i.data(); });

		const auto Result = psi().Message(&PluginId, &Id, Flags & ~FMSG_ALLINONE, HelpTopic.data(), AllItems.data(), AllItems.size(), Buttons.size());
		if (Result != -1)
			return py::integer(Result).release();

		Py_RETURN_NONE;
	}

	static PyObject* InputBox(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 9);

		const auto PluginId = py::cast<UUID>(Args[0]);
		const auto Id = py::cast<UUID>(Args[1]);
		const auto Title = py::cast<std::wstring>(Args[2]);
		const auto SubTitle = py::cast<std::wstring>(Args[3]);
		const auto HistoryName = py::cast<std::wstring>(Args[4]);
		const auto SrcText = py::cast<std::wstring>(Args[5]);
		const auto DestSize = py::cast<size_t>(Args[6]);
		const auto HelpTopic = py::cast<std::wstring>(Args[7]);
		const auto Flags = py::cast<INPUTBOXFLAGS>(Args[8]);
		std::vector<wchar_t> Buffer(DestSize);

		const auto Result = psi().InputBox(&PluginId, &Id, Title.data(), SubTitle.data(), HistoryName.data(), SrcText.data(), Buffer.data(), Buffer.size(), HelpTopic.data(), Flags);
		if (Result)
			return py::string(Buffer.data()).release();

		Py_RETURN_NONE;
	}

	static FarKey PyFarKeyToFarKey(const py::object& PyFarKey)
	{
		return{ py::cast<WORD>(PyFarKey["VirtualKeyCode"]), py::cast<DWORD>(PyFarKey["ControlKeyState"]) };
	}

	static PyObject* Menu(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 12);

		const auto PluginId = py::cast<UUID>(Args[0]);
		const auto Id = py::cast<UUID>(Args[1]);
		const auto X = py::cast<intptr_t>(Args[2]);
		const auto Y = py::cast<intptr_t>(Args[3]);
		const auto MaxHeight = py::cast<intptr_t>(Args[4]);
		const auto Flags = py::cast<FARMENUFLAGS>(Args[5]);
		const auto Title = py::cast<std::wstring>(Args[6]);
		const auto Bottom = py::cast<std::wstring>(Args[7]);
		const auto HelpTopic = py::cast<std::wstring>(Args[8]);

		std::vector<FarKey> OptionalBreakKeys;
		FarKey* BreakKeys = nullptr;

		if (Args[9])
		{
			OptionalBreakKeys = list_to_vector<FarKey>(py::cast<py::list>(Args[9]), PyFarKeyToFarKey);
			BreakKeys = OptionalBreakKeys.data();
		}

		const auto Items = py::cast<py::list>(Args[11]);
		const auto ItemsSize = Items.size();
		std::vector<FarMenuItem> MenuItems;
		MenuItems.reserve(ItemsSize);
		std::vector<std::wstring> MenuStrings;
		MenuStrings.reserve(ItemsSize);

		for(size_t i = 0; i != ItemsSize; ++i)
		{
			auto Item = Items[i];
			FarMenuItem MenuItem{};
			MenuStrings.emplace_back(py::cast<std::wstring>(Item["Text"]));
			MenuItem.Text = MenuStrings.back().data();
			MenuItem.Flags = py::cast<MENUITEMFLAGS>(Item["Flags"]);
			if (const py::object AccelKey = Item["AccelKey"])
			{
				MenuItem.AccelKey = PyFarKeyToFarKey(AccelKey);
			}
			MenuItem.UserData = py::cast<intptr_t>(Item["UserData"]);
			MenuItems.emplace_back(MenuItem);
		}

		intptr_t BreakCode = 0;
		const auto Result = psi().Menu(&PluginId, &Id, X, Y, MaxHeight, Flags, Title.data(), Bottom.data(), HelpTopic.data(), BreakKeys, &BreakCode, MenuItems.data(), MenuItems.size());
		if (Args[10])
		{
			auto BreakCodeContainer = py::cast<py::list>(Args[10]);
			BreakCodeContainer[0] = py::integer(BreakCode);
		}

		if (Result != -1)
			return py::integer(Result).release();

		Py_RETURN_NONE;
	}

	static PyObject* ShowHelp(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 3);

		const auto Flags = py::cast<FARHELPFLAGS>(Args[2]);
		const auto HelpTopic = py::cast<std::wstring>(Args[1]);

		std::wstring ModuleStr;
		GUID ModuleGuid;
		const void* ModulePtr;

		if (Flags & FHELP_GUID)
		{
			ModuleGuid = py::cast<UUID>(Args[0]);
			ModulePtr = &ModuleGuid;
		}
		else
		{
			ModuleStr = py::cast<std::wstring>(Args[0]);
			ModulePtr = &ModuleStr;
		}

		return py::boolean(psi().ShowHelp(static_cast<const wchar_t*>(ModulePtr), HelpTopic.data(), Flags) != FALSE).release();
	}

	static PyObject* AdvControl(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 4);
		
		const auto PluginId = py::cast<UUID>(Args[0]);
		const auto Command = static_cast<ADVANCED_CONTROL_COMMANDS>(py::cast<int>(Args[1]));
		const auto Param1 = py::cast<intptr_t>(Args[2]);
		auto Param2 = Args[3];

		const auto& DefaultCall = [&]
		{
			return psi().AdvControl(&PluginId, Command, Param1, nullptr);
		};

		switch (Command)
		{
		case ACTL_SETCURRENTWINDOW:
		case ACTL_COMMIT:
		case ACTL_REDRAWALL:
		case ACTL_QUIT:
		case ACTL_PROGRESSNOTIFY:
			return py::boolean(DefaultCall() != 0).release();

		case ACTL_GETWINDOWCOUNT:
		case ACTL_GETFARHWND:
			return py::integer(DefaultCall()).release();

		case ACTL_GETFARMANAGERVERSION:
			{
				VersionInfo Info;
				if (!psi().AdvControl(&PluginId, Command, Param1, &Info))
					Py_RETURN_NONE;

				return far_api::type("VersionInfo")(
					py::integer(Info.Major),
					py::integer(Info.Minor),
					py::integer(Info.Revision),
					py::integer(Info.Build),
					far_api::type("VersionStage")(py::integer(Info.Stage))
					).release();
			}

		case ACTL_WAITKEY:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETCOLOR:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETARRAYCOLOR:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETWINDOWINFO:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_SETARRAYCOLOR:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_SYNCHRO:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_SETPROGRESSSTATE:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_SETPROGRESSVALUE:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETFARRECT:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETCURSORPOS:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_SETCURSORPOS:
			// BUGBUG
			Py_RETURN_NONE;

		case ACTL_GETWINDOWTYPE:
			{
				WindowType Type;
				if (!psi().AdvControl(&PluginId, Command, 0, &Type))
					Py_RETURN_NONE;

				auto WindowTypeInstance = far_api::type("WindowType")();
				WindowTypeInstance["Type"] = far_api::type("WindowInfoType")(py::integer(Type.Type));
				return WindowTypeInstance.release();
			}

		default:
			Py_RETURN_NONE;
		}
	}

	static PyObject* PanelControl(PyObject* Self, PyObject* RawArgs)
	{
		const auto Args = get_args(__FUNCTION__, RawArgs, 4);

		const auto Panel = reinterpret_cast<HANDLE>(py::cast<intptr_t>(Args[0]));
		const auto Command = static_cast<FILE_CONTROL_COMMANDS>(py::cast<int>(Args[1]));
		const auto Param1 = py::cast<intptr_t>(Args[2]);
		auto Param2 = Args[3];

		const auto& DefaultCall = [&]
		{
			return psi().PanelControl(Panel, Command, Param1, nullptr);
		};

		switch (Command)
		{
		case FCTL_CLOSEPANEL:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELINFO:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_UPDATEPANEL:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_REDRAWPANEL:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCMDLINE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETCMDLINE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETSELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETVIEWMODE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_INSERTCMDLINE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETUSERSCREEN:
		case FCTL_GETUSERSCREEN:
			return py::boolean(DefaultCall() != 0).release();

		case FCTL_SETPANELDIRECTORY:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETCMDLINEPOS:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCMDLINEPOS:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETSORTMODE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETSORTORDER:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETCMDLINESELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCMDLINESELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_CHECKPANELSEXIST:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETNUMERICSORT:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_ISACTIVEPANEL:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELITEM:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETSELECTEDPANELITEM:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCURRENTPANELITEM:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELDIRECTORY:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCOLUMNTYPES:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETCOLUMNWIDTHS:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_BEGINSELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_ENDSELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_CLEARSELECTION:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETDIRECTORIESFIRST:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELFORMAT:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELHOSTFILE:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETCASESENSITIVESORT:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_GETPANELPREFIX:
			// BUGBUG
			Py_RETURN_NONE;

		case FCTL_SETACTIVEPANEL:
			// BUGBUG
			Py_RETURN_NONE;

		default:
			Py_RETURN_NONE;
		}
	}

	static PyMethodDef Methods[] =
	{
#define FUNC_NAME_VALUE(x) "__" #x, x

		{ FUNC_NAME_VALUE(GetMsg), METH_VARARGS, "Get localised message by Id" },
		{ FUNC_NAME_VALUE(Message), METH_VARARGS, "Show message" },
		{ FUNC_NAME_VALUE(InputBox), METH_VARARGS, "Input box" },
		{ FUNC_NAME_VALUE(Menu), METH_VARARGS, "Menu" },
		{ FUNC_NAME_VALUE(ShowHelp), METH_VARARGS, "Show help" },
		{ FUNC_NAME_VALUE(AdvControl), METH_VARARGS, "Advanced Control Commands" },
		{ FUNC_NAME_VALUE(PanelControl), METH_VARARGS, "Panel Control Commands" },

#undef FUNC_NAME_VALUE
		{}
	};
}

far_api::far_api(const PluginStartupInfo* Psi):
	m_Module(py::import::import("pygin.far"_py)),
	m_Exception("pygin.far.error"),
	m_Psi(*Psi),
	m_Fsf(*Psi->FSF)
{
	m_Psi.FSF = &m_Fsf;

	// We use single instances of PSI and FSF for all plugins.
	// This is perfectly fine for FSF as it is completely static.
	// PSI, however, has some dynamic fields.
	// It is better to reset all of them to avoid any misusing.
	m_Psi.ModuleName = nullptr;
	m_Psi.Private = nullptr;
	m_Psi.Instance = nullptr;

	m_Module.add_object("error", m_Exception);
	m_Module.add_functions(far_api_implementation::Methods);
}

const PluginStartupInfo& far_api::psi() const
{
	return m_Psi;
}
const FarStandardFunctions& far_api::fsf() const
{
	return m_Fsf;
}

const py::object& far_api::get_module() const
{
	return m_Module;
}

static std::unique_ptr<far_api> s_FarApi;

void far_api::initialise(const PluginStartupInfo* Psi)
{
	if (!s_FarApi)
		s_FarApi = std::make_unique<far_api>(Psi);
}

const far_api& far_api::get()
{
	return *s_FarApi;
}

const py::object& far_api::module()
{
	return s_FarApi->get_module();
}

py::type far_api::type(const std::string& TypeName)
{
	return py::type(s_FarApi->m_Module[TypeName.data()]);
}

void far_api::uninitialise()
{
	s_FarApi.reset();
}
