#include <rococo.types.h>
#include <rococo.textures.h>
#include <rococo.imaging.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Textures;

   struct BitmapLocationImpl
   {
      GuiRect uv;
      Vec2i span;
      int32 index;
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
         row->cells.push_back(GridCell{ false, {0, 0}, 0, span.x} );
         rows.push_back(row);
      }

      void AddColumn(int32 width)
      {
         for (auto row : rows)
         {
            auto left = row->cells[row->cells.size()-1].right;
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
            Throw(0, L"Bad index in Grid::Row())");
         }
         return *rows[index];
      }

      const GridRow& operator[](int32 index) const
      {
         if (index >= rows.size())
         {
            Throw(0, L"Bad index in Grid::Row())");
         }
         return *rows[index];
      }
   };

   void PutBitmapInCell(Grid& g, int32 index, Vec2i pos, BitmapLocationImpl& loc)
   {
      auto& row = g[pos.y];
      auto& cell = row.cells[pos.x];

      cell.occupied = true;
      cell.usedSpan = loc.span;
      loc.uv = { cell.left, row.top, cell.left + loc.span.x, row.top + loc.span.y };
      loc.index = index;
      
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
      int i = 0, j = 0;

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

   class TextureArrayBuilder : public ITextureArrayBuilderSupervisor
   {  
   public:
      IResourceLoader& loader;
      ITextureArray& textureArray;

      std::unordered_map<std::wstring, BitmapLocationImpl>  mapNameToLoc;

      struct TextureSpec
      {
         Grid* grid;
      };

      std::vector<TextureSpec> textureSpecs;

      TextureArrayBuilder(IResourceLoader& _loader, ITextureArray& _textureArray) :
         loader(_loader), textureArray(_textureArray)
      {

      }

      virtual void Free()
      {
         Clear();
         delete this;
      }

      virtual void AddBitmap(const wchar_t* name)
      {
         auto i = mapNameToLoc.find(name);
         if (i == mapNameToLoc.end())
         {
             i = mapNameToLoc.insert(std::make_pair(std::wstring(name), BitmapLocationImpl{ GuiRect {0,0,0,0}, {0,0},0 })).first;
         }
      }

      virtual bool TryGetBitmapLocation(const wchar_t* name, BitmapLocation& location)
      {
         auto i = mapNameToLoc.find(name);
         if (i == mapNameToLoc.end())
         {
            location = BitmapLocation{ { 0,0,0,0 }, -1 };
            return false;
         }
         else
         {
            location.textureIndex = i->second.index;
            location.txUV = i->second.uv;
            return true;
         }
      }

      int32 EvaluateRequiredTextureSpan(int32 minWidth)
      {
         struct ANON : public IEventCallback<CompressedTextureBuffer>, public Imaging::IImageLoadEvents 
         {
            const wchar_t* name;
            Vec2i span;
            int32 maxWidth;

            virtual void OnEvent(CompressedTextureBuffer& buffer)
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

            virtual void OnError(const char* message)
            {
               Throw(0, L"Could not load image %s:\n%S", name, message);
            }

            virtual void OnARGBImage(const Vec2i& span, const Imaging::F_A8R8G8B8* data)
            {
               if (span.x <= maxWidth && span.y <= maxWidth)
               {
                  this->span = span;
               }
            }

            virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
            {
               Throw(0, L"Image %s was an alpha, or single channel bitmap.\nOnly 32-bt RGBA or 24-bit RGB files supported.", name);
            }
         } onLoad;

         
         onLoad.maxWidth = textureArray.MaxWidth();

         Vec2i maxSpan = { 0,0 };

         auto i = mapNameToLoc.begin();
         while ( i != mapNameToLoc.end() )
         {
            onLoad.span = { 0,0 };
            onLoad.name = i->first.c_str();
            loader.Load(i->first.c_str(), onLoad);

            if (onLoad.span.x == 0)
            {
               i = mapNameToLoc.erase(i);
            }
            else
            {
               i->second.span = onLoad.span;
               i->second.index = -1;

               maxSpan.x = max(maxSpan.x, onLoad.span.x);
               maxSpan.y = max(maxSpan.y, onLoad.span.y);

               i++;
            }
         }

         int32 maxWidth = max(maxSpan.x, maxSpan.y);

         for (int32 i = 1; i <= textureArray.MaxWidth(); i *= 2)
         {
            if (i > maxWidth)
            {
               maxWidth = i;
               break;
            }
         }

         maxWidth = max(maxWidth, minWidth);

         if (maxWidth > textureArray.MaxWidth())
         {
            Throw(0, L"The span of the largest texture was greater than the largest texture supported");
         }

         return maxWidth;
      }

      void GenNextTexture(int width, int index)
      {
         while (index >= textureArray.TextureCount())
         {
            textureArray.AddTexture();
            auto g = new Grid;
            textureSpecs.push_back(TextureSpec{ g });
         }
      }

      bool TryInsertColumnElseRow(BitmapLocationImpl& bmp, int32 width, Grid& g, int index)
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

      bool TryInsertRowElseColumn(BitmapLocationImpl& bmp, int32 width, Grid& g, int index)
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

            bool GetRelativeOrder(const wchar_t* a, const wchar_t* b)
            {
               auto& A = builder->mapNameToLoc[a];
               auto& B = builder->mapNameToLoc[b];

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

            bool operator()(const wchar_t* a, const wchar_t* b)
            {
               return GetRelativeOrder(a, b);
            }
         } sortBy;

         sortBy.builder = this;

         std::vector<const wchar_t*> orderedNames;
         for (auto& bmp : mapNameToLoc)
         {
            orderedNames.push_back(bmp.first.c_str());
         }
         std::sort(orderedNames.begin(), orderedNames.end(), sortBy);
 
         for (auto& name : orderedNames)
         {
            auto& bmp = mapNameToLoc[name];
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

      virtual void BuildTextures(int32 minWidth, int32 priority)
      {
         int32 width = EvaluateRequiredTextureSpan(minWidth);

         textureArray.ResetWidth(width);

         for (auto& ts : textureSpecs)
         {
            delete ts.grid;
         }

         textureSpecs.clear();

         TileBuildSmallestSquares(width);

         for (auto& i : mapNameToLoc)
         {
            struct ANON : public IEventCallback<CompressedTextureBuffer>, public Imaging::IImageLoadEvents
            {
               const wchar_t* name;
               Vec2i span;
               BitmapLocationImpl* loc;
               ITextureArray* ta;

               virtual void OnEvent(CompressedTextureBuffer& buffer)
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

               virtual void OnError(const char* message)
               {
                  Throw(0, L"Could not load image %s:\n%S", name, message);
               }

               virtual void OnARGBImage(const Vec2i& span, const Imaging::F_A8R8G8B8* data)
               {
                  if (this->span.x != span.x || this->span.y != span.y)
                  {
                     Throw(0, L"Span of image %s changed during execution of the application", name);
                  }

                  Vec2i spanUv = { loc->uv.right - loc->uv.left, loc->uv.bottom - loc->uv.top };
                  if (span.x != spanUv.x || span.y != spanUv.y)
                  {
                     Throw(0, L"Algorithmic error -> BuildTextures(...) loc inconsistent with span");
                  }

                  ta->WriteSubImage(loc->index, data, loc->uv);
               }

               virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
               {
                  Throw(0, L"Image %s was an alpha, or single channel bitmap.\nOnly 32-bt RGBA or 24-bit RGB files supported.", name);
               }
            } onLoad;

            onLoad.name = i.first.c_str();
            onLoad.span = i.second.span;
            onLoad.loc = &i.second;
            onLoad.ta = &textureArray;

            loader.Load(i.first.c_str(), onLoad);
         }
      }

      virtual void Clear()
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

namespace Rococo
{
   namespace Textures
   {
      ITextureArrayBuilderSupervisor* CreateTextureArrayBuilder(IResourceLoader& loader, ITextureArray& textureFactory)
      {
         return new TextureArrayBuilder(loader, textureFactory);
      }
   }
}