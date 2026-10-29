using UnrealBuildTool;

public class MetroKioskEditorTarget : TargetRules
{
	public MetroKioskEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MetroKiosk");
	}
}
