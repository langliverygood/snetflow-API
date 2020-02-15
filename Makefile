#引用全局变量
-include Makefile.env
#编译的最终目标
TARGET = snetflow-API
#需要编译的文件夹
COMPILE_DIR = src

all: subdirs $(TARGET)

#检查文件夹是否存在，若不存在则创建。执行每个需要编译的文件夹中的makefile
subdirs: $(COMPILE_DIR)
	@if [ ! -d $(BUILD_DIR) ]; then mkdir -p $(BUILD_DIR); fi;\
	if [ ! -d $(OBJ_DIR) ]; then mkdir -p $(OBJ_DIR); fi;\
	for dir in $(COMPILE_DIR);do $(MAKE) -C $$dir all||exit 1;done

#链接obj所有的.o 文件
$(TARGET): $(OBJS)
	$(CC) -o $(BUILD_DIR)/$(TARGET) $(OBJS) -L$(LIB_DIR) $(LFLAGS)

OBJS = $(wildcard $(OBJ_DIR)/*.o)

clean:
	@find . -name '*.d.*' -type f -print -exec rm -rf {} \;
	@find . -name '*.d' -type f -print -exec rm -rf {} \;
	rm -rf $(OBJ_DIR)/*.o
	rm -rf $(BUILD_DIR)/$(TARGET)

.PHONY: all clean
