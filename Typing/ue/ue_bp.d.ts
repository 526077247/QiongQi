/// <reference path="puerts.d.ts" />
declare module "ue" {
    import {$Ref, $Nullable} from "puerts"

    import * as cpp from "cpp"

    import * as UE from "ue"

// __TYPE_DECL_START: 613FCDF442375201ABF1C3A6F73D8773
    namespace Game.AssetsPackage.UI.UIMain.Prefabs.UIMainView {
        class UIMainView_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            Image: UE.Image;
            Root: UE.CanvasPanel;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UIMainView_C;
            static Load(InName: string): UIMainView_C;
        
            __tid_UIMainView_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
// __TYPE_DECL_START: 44267F3E49264B831B5F07BFDA02196C
    namespace Game.AssetsPackage.UI.UICommon.Prefabs.UIRoot {
        class UIRoot_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UIRoot_C;
            static Load(InName: string): UIRoot_C;
        
            __tid_UIRoot_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
// __TYPE_DECL_START: 113AAA894F9A36874207A3BAFD20D266
    namespace Game.AssetsPackage.UI.UICommon.Prefabs.RootLoading {
        class RootLoading_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            UberGraphFrame: UE.PointerToUberGraphFrame;
            Loading_Anim: UE.WidgetAnimation;
            ExecuteUbergraph_RootLoading(EntryPoint: number) : void;
            Get_Loading_Brush() : UE.SlateBrush;
            /*
             *Called by both the game and the editor.  Allows users to run initial setup for their widgets to better preview
             *the setup in the designer and since generally that same setup code is required at runtime, it's called there
             *as well.
             *
             ***WARNING**
             *This is intended purely for cosmetic updates using locally owned data, you can not safely access any game related
             *state, if you call something that doesn't expect to be run at editor time, you may crash the editor.
             *
             *In the event you save the asset with blueprint code that causes a crash on evaluation.  You can turn off
             *PreConstruct evaluation in the Widget Designer settings in the Editor Preferences.
             */
            PreConstruct(IsDesignTime: boolean) : void;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): RootLoading_C;
            static Load(InName: string): RootLoading_C;
        
            __tid_RootLoading_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
// __TYPE_DECL_START: 874D453943D1A8EBCCE4C2A0F73AB5A9
    namespace Game.AssetsPackage.UI.UILoading.Prefabs.UILoadingView {
        class UILoadingView_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UILoadingView_C;
            static Load(InName: string): UILoadingView_C;
        
            __tid_UILoadingView_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
}
