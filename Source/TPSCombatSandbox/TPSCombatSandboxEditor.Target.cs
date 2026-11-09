using UnrealBuildTool;

public class TPSCombatSandboxEditorTarget : TargetRules
{
    public TPSCombatSandboxEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
    }
}