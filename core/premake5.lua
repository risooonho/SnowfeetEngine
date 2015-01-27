project "SnowfeetCore"
	platforms { "x32" }
	kind "SharedLib"
	language "C++"
	dependson {
		"Squirrel",
		"JsonBox"
	}
	location "."
	files {
		"**.h",
		"**.hpp",
		"**.cpp"
	}
	links {
		"Squirrel",
		"JsonBox",
		"ws2_32"
	}
	defines {
	}
	filter "configurations:Debug"
		targetdir "../_bin/debug"
		objdir "../_obj/debug"
		defines {
			"SN_BUILD_DEBUG"
		}
	filter "configurations:Release"
		targetdir "../_bin/release"
		objdir "../_obj/release"


