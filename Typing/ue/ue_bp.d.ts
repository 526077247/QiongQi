/// <reference path="puerts.d.ts" />
declare module "ue" {
    import {$Ref, $Nullable} from "puerts"

    import * as cpp from "cpp"

    import * as UE from "ue"

// __TYPE_DECL_START: 0BFD34934B05EF152EE1CB82F2B97674
    namespace Game.AssetsPackage.UI.UIMain.Prefabs.UIMainView {
        class UIMainView_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            Bg: UE.Image;
            Root: UE.CanvasPanel;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UIMainView_C;
            static Load(InName: string): UIMainView_C;
        
            __tid_UIMainView_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
// __TYPE_DECL_START: E5D79EA446C79D957538AB862ACC849C
    namespace Game.AssetsPackage.UI.UICommon.Prefabs.UIRoot {
        class UIRoot_C extends UE.UserWidget {
            constructor(Outer?: Object, Name?: string, ObjectFlags?: number);
            Bg: UE.Image;
            Root: UE.CanvasPanel;
            static StaticClass(): Class;
            static Find(OrigInName: string, Outer?: Object): UIRoot_C;
            static Load(InName: string): UIRoot_C;
        
            __tid_UIRoot_C_0__: boolean;
        }
        
    }

// __TYPE_DECL_END
}
