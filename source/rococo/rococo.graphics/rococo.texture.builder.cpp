#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.textures.h>
#include <rococo.imaging.h>

#include <rococo.hashtable.h>
#include <vector>
#include <algorithm>

#include <rococo.strings.h>

#include <rococo.maths.h>

namespace
{
    using namespace Rococo;
    using namespace Rococo::Strings;
    using namespace Rococo::Graphics;
    using namespace Rococo::Graphics::Textures;

    struct SpriteImpl: ISprite
    {
        GuiRect uv = { 0,0,0,0 };
        Vec2i span = { 0,0 };
        int32 index = -1;
        cstr name = nullptr;
        IdSprite id;

        cstr Name() const override
        {
            return name;
        }

        int TextureIndex() const override
        {
            return index;
        }

        GuiRect TxUV() const override
        {
            return uv;
        }

        Vec2i Span() const override
        {
            return span;
        }

        Rococo::Graphics::IdSprite Id() const override
        {
            return id;
        }
    };

    struct GridCell
    {
        bool occupied;
        Vec2i usedSpan;
        int32 left;
        int32 right;
    };

    struct GridRow
    {
        std::vector<GridCell> cells;
        int32 top;
        int32 bottom;

        GridCell& operator[](int32 index) { return cells[index]; }
    };

    struct Grid
    {
        std::vector<GridRow*> rows;

        ~Grid()
        {
            for (auto& i : rows)
            {
                delete i;
            }
        }

        Vec2i SpanUsed() const
        {
            auto pRow = rows[rows.size() - 1];
            auto& r = *pRow;
            return Vec2i{ r.cells[r.cells.size() - 1].right, r.bottom };
        }

        void MakeFirst(Vec2i span)
        {
            auto* row = new GridRow;
            row->top = 0;
            row->bottom = span.y;
            row->cells.push_back(GridCell{ false, {0, 0}, 0, span.x });
            rows.push_back(row);
        }

        void AddColumn(int32 width)
        {
            for (auto row : rows)
            {
                auto left = row->cells[row->cells.size() - 1].right;
                row->cells.push_back(GridCell{ false, {0,0}, left, left + width });
            }
        }

        void AddRow(int32 height)
        {
            auto& lastRow = *rows[rows.size() - 1];
            auto* row = new GridRow;

            for (size_t i = 0; i < lastRow.cells.size(); ++i)
            {
                const auto& src = lastRow.cells[i];
                row->cells.push_back(GridCell{ false, {0,0}, src.left, src.right });
                row->top = lastRow.bottom;
                row->bottom = row->top + height;
            }
            rows.push_back(row);
        }

        GridRow& operator[](int32 index)
        {
            if (index >= rows.size())
            {
                Throw(0, "Bad index in Grid::Row())");
            }
            return *rows[index];
        }

        const GridRow& operator[](int32 index) const
        {
            if (index >= rows.size())
            {
                Throw(0, "Bad index in Grid::Row())");
            }
            return *rows[index];
        }
    };

    void PutBitmapInCell(Grid& g, int32 index, Vec2i pos, SpriteImpl& sprite)
    {
        auto& row = g[pos.y];
        auto& cell = row.cells[pos.x];

        cell.occupied = true;
        cell.usedSpan = sprite.span;
        sprite.uv = { cell.left, row.top, cell.left + sprite.span.x, row.top + sprite.span.y };
        sprite.index = index;

    }

    bool IsColumnWideEnough(int32 width, const Grid& g)
    {
        auto& r = g[0];
        for (auto& cell : r.cells)
        {
            if ((cell.right - cell.left) >= width)
            {
                return true;
            }
        }

        return false;
    }

    bool IsRowTallEnough(int32 height, const Grid& g)
    {
        for (auto& pr : g.rows)
        {
            auto& r = *pr;
            if ((r.bottom - r.top) >= height)
            {
                return true;
            }
        }

        return false;
    }

    Vec2i GetFirstFit(Vec2i span, const Grid& g)
    {
        int j = 0;

        for (auto pRow : g.rows)
        {
            auto& r = *pRow;

            int i = 0;

            if ((r.bottom - r.top) >= span.y)
            {
                for (auto& cell : r.cells)
                {
                    if (!cell.occupied && (cell.right - cell.left) >= span.x)
                    {
                        return Vec2i{ i, j };
                    }

                    i++;
                }
            }

            j++;
        }

        return{ -1,-1 };
    }

    class TextureArrayBuilder : public IBitmapArrayBuilderSupervisor
    {
    public:
        ICompressedResourceLoader& loader;
        IBitmapArray& textureArray;

        int newBitmaps = 0;

        stringmap<SpriteImpl*> nameToSprite;
        std::vector<SpriteImpl*> idToSprite;

        struct TextureSpec
        {
            Grid* grid;
        };

        std::vector<TextureSpec> textureSpecs;

        TextureArrayBuilder(ICompressedResourceLoader& _loader, IBitmapArray& _textureArray) :
            loader(_loader), textureArray(_textureArray)
        {

        }

        virtual ~TextureArrayBuilder()
        {
            for (auto& i : nameToSprite)
            {
                delete i.second;
            }
        }
        
        void Free() override
        {
            Clear();
            delete this;
        }

        void AddBitmap(cstr name) override
        {
            auto i = nameToSprite.insert(name, nullptr);
            if (i.second)
            {
                newBitmaps++;
                i.first->second = new SpriteImpl();
                i.first->second->name = i.first->first;

                idToSprite.push_back(i.first->second);

                // Here we define the mapping of SpriteId to sprite* starts at id 1.
                i.first->second->id.value = (int32)idToSprite.size();
            }
        }

        ISprite* FindBitmap(cstr name) const override
        {
            if (name == nullptr)
            {
                Throw(0, "FindBitmap: name was null");
            }

            auto i = nameToSprite.find(name);
            if (i == nameToSprite.end())
            {
                return nullptr;
            }
            else
            {
                return i->second;
            }
        }

        ISprite* FindBitmapById(IdSprite id) const override
        {
            size_t zeroIndex = (size_t)(id.value - 1);
            if (zeroIndex >= idToSprite.size())
            {
                return nullptr;
            }

            return idToSprite[zeroIndex];
        }

        int32 EvaluateRequiredTextureSpan(int32 minWidth, IEventCallback<BitmapUpdate>* onUpdate)
        {
            struct ANON : public IEventCallback<CompressedTextureBuffer>, public Imaging::IImageLoadEvents
            {
                cstr name;
                Vec2i span;
                int32 maxWidth;

                void OnEvent(CompressedTextureBuffer& buffer) override
                {
                    if (buffer.type == COMPRESSED_TYPE_JPG)
                    {
                        Imaging::DecompressJPeg(*this, buffer.data, buffer.nBytes);
                    }
                    else
                    {
                        Imaging::DecompressTiff(*this, buffer.data, buffer.nBytes);
                    }
                }

                void OnError(const char* message) override
                {
                    Throw(0, "Could not load image %s:\n%s", name, message);
                }

                void OnRGBAImage(const Vec2i& span, const RGBAb*) override
                {
                    if (span.x <= maxWidth && span.y <= maxWidth)
                    {
                        this->span = span;
                    }
                    else
                    {
                        Throw(0, "Image span (%d,%d) for %s exceeded maximum dimension of (%d, %d)", span.x, span.y, name, maxWidth, maxWidth);
                    }
                }

                void OnAlphaImage(const Vec2i&, const uint8*) override
                {
                    Throw(0, "Image %s was an alpha, or single channel bitmap.\nOnly 32-bt RGBA or 24-bit RGB files supported.", name);
                }
            } onLoad;

            onLoad.maxWidth = textureArray.MaxWidth();

            Vec2i maxSpan = { 0,0 };

            for(auto& i: nameToSprite)
            {
                onLoad.span = { 0,0 };
                onLoad.name = i.first;

                try
                {
                    loader.Load(i.first, onLoad);

                    if (onUpdate)
                    {
                        BitmapUpdate bu;
                        bu.name = i.first;
                        bu.msg = "";
                        bu.hr = 0;
                        onUpdate->OnEvent(bu);
                    }
                }
                catch (IException& ex)
                {
                    if (onUpdate)
                    {
                        BitmapUpdate bu;
                        bu.name = i.first;
                        bu.msg = ex.Message();
                        bu.hr = ex.ErrorCode();
                        onUpdate->OnEvent(bu);
                    }

                    throw;
                }

                if (onLoad.span.x > textureArray.MaxWidth() || onLoad.span.y > textureArray.MaxWidth())
                {
                    Throw(0, "The span of %s was greater than the largest texture supported (%u x %u)", onLoad.name, textureArray.MaxWidth(), textureArray.MaxWidth());
                }

                i.second->span = onLoad.span;
                i.second->index = -1;

                maxSpan.x = max(maxSpan.x, onLoad.span.x);
                maxSpan.y = max(maxSpan.y, onLoad.span.y);
            }

            int32 maxWidth = max(maxSpan.x, maxSpan.y);

            for (int32 j = 1; j <= textureArray.MaxWidth(); j *= 2)
            {
                if (j >= maxWidth)
                {
                    maxWidth = j;
                    break;
                }
            }

            maxWidth = max(maxWidth, minWidth);

            if (maxWidth > textureArray.MaxWidth())
            {
                Throw(0, "The span of the largest texture was greater than the largest texture supported");
            }

            return maxWidth;
        }

        void GenNextTexture(int width, int index)
        {
            UNUSED(width);
            while (index >= textureArray.TextureCount())
            {
                textureArray.AddTexture();
                auto g = new Grid;
                textureSpecs.push_back(TextureSpec{ g });
            }
        }

        bool TryInsertColumnElseRow(SpriteImpl& bmp, int32 width, Grid& g, int index)
        {
            Vec2i spanUsed = g.SpanUsed();

            if (IsRowTallEnough(bmp.span.y, g))
            {
                if (bmp.span.x + g.SpanUsed().x <= width)
                {
                    g.AddColumn(bmp.span.x);
                    PutBitmapInCell(g, index, GetFirstFit(bmp.span, g), bmp);
                    return true;
                }
                else if (IsColumnWideEnough(bmp.span.x, g) && bmp.span.y + spanUsed.y <= width)
                {
                    g.AddRow(bmp.span.y);
                    PutBitmapInCell(g, index, GetFirstFit(bmp.span, g), bmp);
                    return true;
                }
            }

            return false;
        }

        bool TryInsertRowElseColumn(SpriteImpl& bmp, int32 width, Grid& g, int index)
        {
            Vec2i spanUsed = g.SpanUsed();

            if (IsColumnWideEnough(bmp.span.x, g))
            {
                if (bmp.span.y + spanUsed.y <= width)
                {
                    g.AddRow(bmp.span.y);
                    PutBitmapInCell(g, index, GetFirstFit(bmp.span, g), bmp);
                    return true;
                }
                else if (IsRowTallEnough(bmp.span.y, g) && bmp.span.y + spanUsed.y <= width)
                {
                    g.AddRow(bmp.span.y);
                    PutBitmapInCell(g, index, GetFirstFit(bmp.span, g), bmp);
                    return true;
                }
            }

            return false;
        }

        void TileBuildSmallestSquares(int width)
        {
            struct ANON
            {
                TextureArrayBuilder* builder;

                bool GetRelativeOrder(cstr a, cstr b)
                {
                    auto& A = *builder->nameToSprite[a];
                    auto& B = *builder->nameToSprite[b];

                    if (A.span.x > A.span.y)
                    {
                        // A row like object always comes before a column like object or square object
                        if (B.span.x <= B.span.y)
                        {
                            return true;
                        }
                        else
                        {
                            return A.span.x > B.span.x;
                        }
                    }
                    else if (A.span.x < B.span.y)
                    {
                        if (B.span.x >= B.span.y)
                        {
                            return false;
                        }
                        else
                        {
                            return A.span.y > B.span.y;
                        }
                    }
                    else
                    {
                        if (B.span.x != B.span.y)
                        {
                            return false;
                        }

                        return A.span.x > B.span.x;
                    }
                }

                bool operator()(cstr a, cstr b)
                {
                    return GetRelativeOrder(a, b);
                }
            } sortBy;

            sortBy.builder = this;

            std::vector<cstr> orderedNames;
            for (auto& bmp : nameToSprite)
            {
                orderedNames.push_back(bmp.first);
            }
            std::sort(orderedNames.begin(), orderedNames.end(), sortBy);

            for (auto& name : orderedNames)
            {
                auto& bmp = *nameToSprite[name];
                for (int32 i = 0; i < textureArray.TextureCount(); ++i)
                {
                    auto& ts = textureSpecs[i];
                    Grid& g = *ts.grid;
                    Vec2i spanUsed = g.SpanUsed();

                    Vec2i pos = GetFirstFit(bmp.span, g);
                    if (pos.x >= 0)
                    {
                        PutBitmapInCell(g, i, pos, bmp);
                        break;
                    }

                    if (bmp.span.x < bmp.span.y)
                    {
                        if (TryInsertColumnElseRow(bmp, width, g, i))
                        {
                            break;
                        }
                        if (TryInsertRowElseColumn(bmp, width, g, i))
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (TryInsertRowElseColumn(bmp, width, g, i))
                        {
                            break;
                        }
                        if (TryInsertColumnElseRow(bmp, width, g, i))
                        {
                            break;
                        }
                    }

                    if (bmp.span.x + spanUsed.x <= width && bmp.span.y + spanUsed.y <= width)
                    {
                        g.AddRow(bmp.span.y);
                        g.AddColumn(bmp.span.x);
                        PutBitmapInCell(g, i, GetFirstFit(bmp.span, g), bmp);
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }

                if (bmp.index < 0)
                {
                    int32 index = (int32)textureArray.TextureCount();
                    GenNextTexture(width, index);

                    textureSpecs[index].grid->MakeFirst(bmp.span);

                    PutBitmapInCell(*textureSpecs[index].grid, index, { 0,0 }, bmp);
                }
            }
        }

        struct SubImageWriterOnLoad : public IEventCallback<CompressedTextureBuffer>, public Imaging::IImageLoadEvents
        {
            cstr name;
            Vec2i span;
            SpriteImpl* loc;
            IBitmapArray* ta;

            void OnEvent(CompressedTextureBuffer& buffer) override
            {
                if (buffer.type == COMPRESSED_TYPE_JPG)
                {
                    Imaging::DecompressJPeg(*this, buffer.data, buffer.nBytes);
                }
                else
                {
                    Imaging::DecompressTiff(*this, buffer.data, buffer.nBytes);
                }
            }

            void OnError(const char* message) override
            {
                Throw(0, "Could not load image %s:\n%s", name, message);
            }

            void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
            {
                if (this->span.x != span.x || this->span.y != span.y)
                {
                    Throw(0, "Span of image %s changed during execution of the application", name);
                }

                Vec2i spanUv = { loc->uv.right - loc->uv.left, loc->uv.bottom - loc->uv.top };
                if (span.x != spanUv.x || span.y != spanUv.y)
                {
                    Throw(0, "Algorithmic error -> BuildTextures(...) loc inconsistent with span");
                }

                ta->WriteSubImage(loc->index, data, loc->uv);
            }

            void OnAlphaImage(const Vec2i&, const uint8*) override
            {
                Throw(0, "Image %s was an alpha, or single channel bitmap.\nOnly 32-bt RGBA or 24-bit RGB files supported.", name);
            }
        };

        void WriteSubImages()
        {
            for (auto& i : nameToSprite)
            {
                SubImageWriterOnLoad onLoad;
                onLoad.name = i.first;
                onLoad.span = i.second->span;
                onLoad.loc = i.second;
                onLoad.ta = &textureArray;

                loader.Load(i.first, onLoad);
            }
        }

        void BuildBitmaps(int32 minWidth, IEventCallback<BitmapUpdate>* onUpdate = nullptr) override
        {
            if (newBitmaps == 0)
            {
                return;
            }

            if (textureArray.TextureCount() != 0)
            {
                textureArray.Clear();
            }

            int32 width = EvaluateRequiredTextureSpan(minWidth, onUpdate);

            textureArray.ResetWidth(width);

            for (auto& ts : textureSpecs)
            {
                delete ts.grid;
            }

            textureSpecs.clear();

            TileBuildSmallestSquares(width);

            WriteSubImages();

            newBitmaps = 0;
        }

        void Clear() override
        {
            textureArray.ResetWidth(0);

            for (auto& t : textureSpecs)
            {
                delete t.grid;
            }

            textureSpecs.clear();
        }

    };
}

namespace Rococo::Graphics::Textures
{
    ROCOCO_API_EXPORT void StandardLoadFromCompressedTextureBuffer(cstr name, IEventCallback<CompressedTextureBuffer>& onLoad, IO::IInstallation& installation, IExpandingBuffer& buffer)
    {
        COMPRESSED_TYPE type;

        auto* ext = GetFileExtension(name);
        if (Eq(ext, ".tiff") || Eq(ext, ".tif"))
        {
            type = COMPRESSED_TYPE_TIF;
        }
        else if (Eq(ext, ".jpg") || Eq(ext, ".jpeg"))
        {
            type = COMPRESSED_TYPE_JPG;
        }
        else
        {
            Throw(0, "%s: Image files either be a tif or a jpg.", name);
        }

        installation.LoadResource(name, buffer, 64_megabytes);
        CompressedTextureBuffer args =
        {
           buffer.GetData(),
           buffer.Length(),
           type
        };
        onLoad.OnEvent(args);
    }

    ROCOCO_API_EXPORT IBitmapArrayBuilderSupervisor* CreateBitmapArrayBuilder(ICompressedResourceLoader& loader, IBitmapArray& textureFactory)
    {
        return new TextureArrayBuilder(loader, textureFactory);
    }
}