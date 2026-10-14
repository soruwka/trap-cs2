import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
NS = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}


class CoreTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="nocturne-tests-")
        cls.addClassCleanup(cls.directory.cleanup)
        cls.compiler = shlex.split(os.environ.get("CXX", "c++"))
        cls.flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic"]
        cls.flags += shlex.split(os.environ.get("CXXFLAGS", ""))
        cls.binary = Path(cls.directory.name) / "core_tests"
        subprocess.run(
            cls.compiler + cls.flags + [
                "-I", str(ROOT / "src"),
                str(ROOT / "tests/core_tests.cpp"),
                str(ROOT / "src/backend/keybinding/KeyHandler.cpp"),
                "-o", str(cls.binary),
            ],
            check=True,
        )

    def run_case(self, name):
        subprocess.run([str(self.binary), name], check=True, timeout=30)

    def test_vectors(self):
        self.run_case("vectors")

    def test_keybindings(self):
        self.run_case("keybindings")

    def test_keybinding_registration_during_dispatch(self):
        self.run_case("keybinding_registration")

    def test_feature_ownership(self):
        self.run_case("features")

    def test_feature_registration_during_tick(self):
        self.run_case("feature_registration")

    def test_headers_are_self_contained(self):
        for header in (
            "classes/Vector.hpp",
            "backend/keybinding/Keybind.hpp",
            "backend/keybinding/KeyHandler.hpp",
            "feature/Feature.hpp",
        ):
            with self.subTest(header=header):
                subprocess.run(
                    self.compiler + self.flags + [
                        "-I", str(ROOT / "src"), "-x", "c++", "-fsyntax-only", "-",
                    ],
                    input=f'#include <{header}>\n', text=True, check=True,
                )

    def test_freetype_headers_are_self_contained(self):
        source = (
            "#include <ft2build.h>\n"
            "#include FT_FREETYPE_H\n"
            "#include FT_MODULE_H\n"
            "#include FT_GLYPH_H\n"
            "#include FT_SYNTHESIS_H\n"
        )
        c_flags = ["-std=c17", "-Wall", "-Wextra", "-Werror", "-pedantic"]
        c_flags += shlex.split(os.environ.get("CFLAGS", ""))
        compilers = (
            ("c", shlex.split(os.environ.get("CC", "cc")), c_flags),
            ("c++", self.compiler, self.flags),
        )
        for language, compiler, flags in compilers:
            with self.subTest(language=language):
                subprocess.run(
                    compiler + flags + [
                        "-I", str(ROOT / "thirdparty/imgui/freetype/include"),
                        "-x", language, "-fsyntax-only", "-",
                    ],
                    input=source, text=True, check=True,
                )


class ProjectTests(unittest.TestCase):
    def setUp(self):
        self.project = ET.parse(ROOT / "Nocturne.vcxproj").getroot()

    def test_project_and_filter_files_exist(self):
        for name in ("Nocturne.vcxproj", "Nocturne.vcxproj.filters"):
            document = ET.parse(ROOT / name).getroot()
            for tag in ("ClCompile", "ClInclude"):
                for item in document.findall(f".//ms:{tag}[@Include]", NS):
                    with self.subTest(manifest=name, include=item.attrib["Include"]):
                        self.assertTrue((ROOT / item.attrib["Include"].replace("\\", "/")).is_file())

    def test_branding_and_solution_reference(self):
        solution = (ROOT / "Nocturne.sln").read_text(encoding="utf-8-sig")
        self.assertIn('= "Nocturne", "Nocturne.vcxproj"', solution)
        self.assertEqual(self.project.find(".//ms:RootNamespace", NS).text, "Nocturne")
        self.assertEqual(self.project.find(".//ms:TargetName", NS).text, "Nocturne")
        self.assertNotIn("TRAPCS2", (ROOT / "Nocturne.vcxproj").read_text())
        gui = (ROOT / "src/gui/gui.cpp").read_text()
        self.assertIn('ImGui::Begin( "Nocturne"', gui)
        self.assertIn('0.f, "Nocturne"', gui)

    def test_only_supported_configurations_are_exposed(self):
        configs = self.project.findall(".//ms:ProjectConfiguration", NS)
        self.assertEqual({item.attrib["Include"] for item in configs}, {"Debug|x64", "Release|x64"})
        solution = (ROOT / "Nocturne.sln").read_text(encoding="utf-8-sig")
        self.assertNotIn("x86", solution)
        self.assertNotIn("Win32", solution)

    def test_shared_compile_settings(self):
        common = next(group for group in self.project.findall("ms:ItemDefinitionGroup", NS)
                      if "Condition" not in group.attrib)
        compile_settings = common.find("ms:ClCompile", NS)
        self.assertEqual(compile_settings.find("ms:LanguageStandard", NS).text, "stdcpp17")
        self.assertEqual(compile_settings.find("ms:PrecompiledHeader", NS).text, "NotUsing")
        self.assertIn("%(AdditionalIncludeDirectories)", compile_settings.find("ms:AdditionalIncludeDirectories", NS).text)
        self.assertNotIn("/FORCE:MULTIPLE", (ROOT / "Nocturne.vcxproj").read_text())
        self.assertNotIn("pch.h", (ROOT / "Nocturne.vcxproj").read_text())

    def test_freetype_build_matches_runtime(self):
        target = self.project.find("ms:Target[@Name='BuildFreeType']", NS)
        self.assertEqual(target.attrib["BeforeTargets"], "ClCompile")
        task = target.find("ms:MSBuild", NS)
        self.assertIn("Configuration=$(Configuration) Static", task.attrib["Properties"])
        path = self.project.find(".//ms:FreeTypeProject", NS).text
        freetype_path = ROOT / path.replace("$(ProjectDir)", "").replace("\\", "/")
        freetype = ET.parse(freetype_path).getroot()
        for config, runtime in (("Debug", "MultiThreadedDebug"), ("Release", "MultiThreaded")):
            condition = f"'$(Configuration)|$(Platform)'=='{config}|x64'"
            main_group = next(g for g in self.project.findall("ms:ItemDefinitionGroup", NS)
                              if g.attrib.get("Condition") == condition)
            self.assertEqual(main_group.find("ms:ClCompile/ms:RuntimeLibrary", NS).text, runtime)
            ft_condition = condition.replace(f"{config}|", f"{config} Static|")
            ft_group = next(g for g in freetype.findall("ms:ItemDefinitionGroup", NS)
                            if g.attrib.get("Condition") == ft_condition)
            self.assertEqual(ft_group.find("ms:ClCompile/ms:RuntimeLibrary", NS).text, runtime)


if __name__ == "__main__":
    unittest.main()
