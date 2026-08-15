import { Log } from "../../../Mono/Module/Log/Log";
import { IOnCreate } from "../UI/IOnCreate";
import { IOnDestroy } from "../UI/IOnDestroy";
import { UIBaseComponent } from "../UI/UIBaseComponent";
import * as string from "../../../Mono/Helper/StringHelper"
import { Color, Image, PaperSprite, LinearColor, Texture2D, SlateColor, Vector2D } from "ue";
import { ImageLoaderManager } from "../Resource/ImageLoaderManager";

export class UIImage extends UIBaseComponent implements IOnDestroy, IOnCreate<string> {

    public getConstructor(){
        return UIImage;
    }

    private spritePath: string;
    private image: Image;
    private isSetSprite: boolean;
    private version: number = 0;
    private cacheUrl: string;

    public onCreate(path: string)
    {
        this.setSpritePath(path);
    }

    public onDestroy()
    {
        this.version++;
        if (!string.isNullOrEmpty(this.spritePath))
        {
            this.image.SetBrush(null);
            ImageLoaderManager.instance?.releaseImage(this.spritePath);
            this.spritePath = null;
        }

        if (this.isSetSprite)
        {
            this.image.SetBrush(null);
            this.isSetSprite = false;
        }
        
        if (!string.isNullOrEmpty(this.cacheUrl))
        {
            ImageLoaderManager.instance?.releaseOnlineImage(this.cacheUrl);
            this.cacheUrl = null;
        }
    }

    private activatingComponent()
    {
        if (this.image == null)
        {
            const widget = this.getWidget();
            if (!(widget instanceof Image))
            {
                Log.error(`添加UI侧组件UIImage时，物体${widget.GetName()}不是Image组件`);
            }
            else
            {
                this.image = widget as Image;
            }
        }
    }

    /**
     * 设置图片地址（注意尽量不要和SetOnlineSpritePath混用
     * @param spritePath 
     * @param setNativeSize 
     * @returns 
     */
    public async setSpritePath(spritePath: string, setNativeSize: boolean = false): Promise<void>
    {
        this.version++;
        const thisVersion = this.version;
        if (spritePath == this.spritePath && !this.isSetSprite)
        {
            return;
        }
        this.activatingComponent();
        // if (this.bgAutoFit != null) this.bgAutoFit.enabled = false;
        var baseSpritePath = this.spritePath;

        if (string.isNullOrEmpty(spritePath))
        {
            this.image.SetBrush(null);
            this.isSetSprite = false;
            this.spritePath = spritePath;
        }
        else
        {
            var sprite: PaperSprite = await ImageLoaderManager.instance.loadSpriteAsync(spritePath);
            if (thisVersion != this.version)
            {
                ImageLoaderManager.instance.releaseImage(spritePath);
                return;
            }
            this.spritePath = spritePath;
            this.image.SetBrushResourceObject(sprite);
            this.isSetSprite = false;
            if(setNativeSize)
                this.setNativeSize();
            // if (this.bgAutoFit != null)
            // {
            //     this.bgAutoFit.SetSprite(sprite);
            //     this.bgAutoFit.enabled = true;
            // }
        }
        if(!string.isNullOrEmpty(baseSpritePath))
            ImageLoaderManager.instance.releaseImage(baseSpritePath);
       
    }

    /**
     * 设置网络图片地址（注意尽量不要和SetSpritePath混用
     * @param spritePath 
     * @param setNativeSize 
     * @param defaultSpritePath 
     */
    public async setOnlineSpritePath(url: string, setNativeSize: boolean = false, defaultSpritePath: string = null)
    {
        this.activatingComponent();
        if (!string.isNullOrEmpty(defaultSpritePath))
        {
            await this.setSpritePath(defaultSpritePath, setNativeSize);
        }
        this.version++;
        const thisVersion = this.version;
        var sprite = await ImageLoaderManager.instance.getOnlineTexture(url);
        if (sprite != null)
        {
            if (thisVersion != this.version)
            {
                ImageLoaderManager.instance.releaseOnlineImage(url);
                return;
            }
            this.setSprite(sprite);
            if (setNativeSize)
            {
                this.setNativeSize();
            }
            if (!string.isNullOrEmpty(this.cacheUrl))
            {
                ImageLoaderManager.instance.releaseOnlineImage(this.cacheUrl);
                this.cacheUrl = null;
            }
            this.cacheUrl = url;
        }
    }

    /**
     * 按照当前图片资源的原始像素尺寸设置组件大小（需先调用 setSprite / setSpritePath / setOnlineSpritePath 设置资源）
     */
    public setNativeSize()
    {
        if (this.image == null)
        {
            return;
        }
        const brush = this.image.Brush;
        if (brush == null)
        {
            return;
        }
        const resourceObject = brush.ResourceObject;
        let width: number = 0;
        let height: number = 0;
        if (resourceObject instanceof PaperSprite)
        {
            // PaperSprite 的 BakedSourceDimension 即 Sprite 在源纹理中的像素尺寸（图集内为对应区域尺寸，非整图）
            width = resourceObject.BakedSourceDimension.X;
            height = resourceObject.BakedSourceDimension.Y;
        }
        else if (resourceObject instanceof Texture2D)
        {
            width = resourceObject.Blueprint_GetSizeX();
            height = resourceObject.Blueprint_GetSizeY();
        }
        if (width <= 0 || height <= 0)
        {
            return;
        }
        brush.ImageSize.X = width;
        brush.ImageSize.Y = height;
        // 覆盖 DesiredSize，使布局（如按期望尺寸排列的容器）按原生尺寸生效
        this.image.SetDesiredSizeOverride(new Vector2D(width, height));
    }

    public getSpritePath()
    {
        return this.spritePath;
    }

    public setColor(color: string | Color | LinearColor| SlateColor)
    {
        if(color instanceof Color){
            this.activatingComponent();
            this.image.SetColorAndOpacity(new LinearColor(color));
            return;
        }
        if(color instanceof LinearColor){
            this.activatingComponent();
            this.image.SetColorAndOpacity(color);
            return;
        }
        if(color instanceof SlateColor){
            this.activatingComponent();
            this.image.SetColorAndOpacity(color.SpecifiedColor);
            return;
        }
        if(string.isNullOrEmpty(color)) return;
        this.activatingComponent();
        const colorRgb = Color.FromHex(color);
        this.image.SetColorAndOpacity(new LinearColor(colorRgb));
    }

    public getColor(): Color
    {
        this.activatingComponent();
        return this.image.ColorAndOpacity.ToRGBE();
    }

    public setImageAlpha(a: number)
    {
        this.activatingComponent();
        const color = this.image.ColorAndOpacity;
        color.A = a;
        this.image.SetColorAndOpacity(color);
    }
    
    public setSprite(sprite: PaperSprite|Texture2D)
    {
        this.activatingComponent();
        if(sprite instanceof Texture2D){
            this.image.SetBrushFromTexture(sprite);
        }else{
            this.image.SetBrushResourceObject(sprite);
        }
        this.isSetSprite = true;
    }
}